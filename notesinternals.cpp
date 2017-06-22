#include "notesinternals.h"



NotesInternals::NotesInternals(QObject *parent) : QObject(parent),hashFunction_("sha256"),currentCategory_(invalidCategory()),currentEntry_(invalidEntry()),encryptionEnabled_(false)
{
    QCA::init();
    if(!QDir("./plain").exists())
        QDir(".").mkdir("plain");
    if(!QDir("./enc").exists())
        QDir(".").mkdir("enc");
    loadUnencryptedCategories();
}

const Category NotesInternals::addCategory(QString categoryName)
{
    std::shared_ptr<CategoryContent> content(new CategoryContent());
    Category ret=Category(categoryName,QDateTime::currentDateTime(),content);
    categorySet_.insert(ret);

    //set encryption according to current encryption state
    content->encrypted_=encryptionEnabled_;

    //set empty folder name, update to create new category folder and file
    content->path_="";
    updateCategoryFile(ret);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //return new category
    return ret;
}

bool NotesInternals::removeCategory(Category &category)
{
    //return false if category is invalid
    if(!isValid(category))
        return false;

    //remove category directory and files therein
    QDir dir(getPath(category));
    dir.removeRecursively();

    //delete category object. entries contained in category will be cleaned up by destructor
    //delete getContent_(category);

    //remove from set
    categorySet_.erase(category);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //if the deleted category was selected, select invalid category instead
    if(currentCategory_==category)
        selectCategory(invalidCategory());

    return true;
}

const Category NotesInternals::renameCategory(Category &category, QString newCategoryName)
{
    //return invalid if passed category is invalid
    if(!isValid(category))
        return invalidCategory();

    categorySet_.erase(category);
    //create new category with newCategoryName and content, insert into category set
    Category ret=Category(newCategoryName,category.date,category.content);
    categorySet_.insert(ret);

    //update category file and folder
    updateCategoryFile(ret);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //if the renamed category was selected, retain selection of the renamed created category
    if(currentCategory_==category)
        selectCategory(ret);

    return ret;
}

const Category NotesInternals::toggleCategoryEncryption(Category category)
{
    //return invalid category if passed category is invalid or encryption is not active
    if(!isValid(category) || !encryptionEnabled_)
        return invalidCategory();

    //change encryption setting
    category.content->encrypted_=!category.content->encrypted_;

    //update category file and folder
    updateCategoryFile(category);

    //update all entry files, to make sure they're encrypted / decrypted according to the new setting
    for(EntrySet::iterator i=category.content->entrySet_.begin();i!=category.content->entrySet_.end();++i)
        updateEntryFile(category,*i);

    //set of categories was altered. entries not necessarily (UI doesn't care about new files)
    emit categoryListChanged();

    //selection is not altered by these internal changes

    return category;
}

const Entry NotesInternals::addEntry(Category &category, QString entryName)
{
    //return invalid if passed category is invalid
    if(!isValid(category))
        return invalidEntry();

    //create entry objec
    std::shared_ptr<EntryContent> content(new EntryContent());
    //create entry from name, current time, content. insert entry into category
    Entry ret=Entry(entryName,QDateTime::currentDateTime(),content);
    category.content->entrySet_.insert(ret);

    //set empty file name, update to create new entry file
    content->fileName_="";
    updateEntryFile(category,ret);

    //if category is currently selected, notify GUI of changes
    if(currentCategory_==category)
        emit categoryContentChanged();
    return ret;
}

bool NotesInternals::removeEntry(Category &category, Entry &entry)
{
    //return false if (category,entry) don't represent a valid entry
    if(!isValid(category,entry))
        return false;

    //remove entry file
    QFile file(getPath(category)+getFileName(entry));
    file.remove();

    //remove entry from category
    category.content->entrySet_.erase(entry);

    //if category is currently selected, notify GUI
    if(currentCategory_==category)
    {
        emit categoryContentChanged();
        //if entry was currently selected, select invalid instead
        if(currentEntry_==entry)
            selectEntry(invalidEntry());
    }
    return true;
}

const Entry NotesInternals::renameEntry(Category &category, Entry &entry, QString newEntryName)
{
    //return false if (category,entry) don't represent a valid entry
    if(!isValid(category,entry))
        return invalidEntry();

    //remove entry from category
    category.content->entrySet_.erase(entry);
    //create entry with new name, creation date and content, place it into the entry set of the category
    Entry ret=Entry(newEntryName,entry.date,entry.content);
    category.content->entrySet_.insert(ret);

    //update entry file to reflect name change
    updateEntryFile(category,ret);

    //if category is currently selected, notify UI
    if(currentCategory_==category)
    {
        emit categoryContentChanged();
        //if entry was currently selected, select the newly created one
        if(currentEntry_==entry)
            selectEntry(ret);
    }

    //return entry with new name
    return ret;
}

const Entry NotesInternals::moveEntry(Category &oldCategory, Entry &entry, Category &newCategory)
{
    //if either (oldCategory,entry) don't represent a valid entry or newCategory does not represent a category, return invalid
    if(!isValid(oldCategory,entry) || !isValid(newCategory))
        return invalidEntry();

    //remove entry from old category, insert into new one
    oldCategory.content->entrySet_.erase(entry);
    newCategory.content->entrySet_.insert(entry);

    //rename entry file, move it to new category folder, update fileName_
    QDir("./").rename(getPath(oldCategory)+entry.content->fileName_,getPath(newCategory)+entry.content->fileName_+".tmp");
    entry.content->fileName_=entry.content->fileName_+".tmp";

    //update entry file, which means delete the moved and renamed version, create a new one
    //(deals with both possible existing file of the same name as well as the case where one category is encrypted and the other is not)
    updateEntryFile(newCategory,entry);

    //if currently selected category is either the previous or new category of the moved entry, notify UI of changes
    if(currentCategory_==oldCategory || currentCategory_==newCategory)
    {
        emit categoryContentChanged();
        //if entry was currently selected, select invalid instead
        if(currentCategory_==oldCategory && currentEntry_==entry)
            selectEntry(invalidEntry());
    }

    //return entry passed (no internal changes have occured)
    return entry;
}

const Entry NotesInternals::modifyEntryText(Category category, Entry entry, QCA::SecureArray newEntryText)
{
    //return invalid entry in case (category,entry) does not represent a valid entry
    if(!isValid(category,entry))
        return invalidEntry();

    //update entry text
    entry.content->text_=newEntryText;

    //update entry file with new text
    updateEntryFile(category,entry);

    //notify UI if entry is currently selected
    if(currentCategory_==category && currentEntry_==entry)
        emit entryContentChanged();

    //return entry (above changes do not affect the entry object directly)
    return entry;
}

bool NotesInternals::enableEncryption(const QCA::SecureArray &password)
{
    if(encryptionEnabled_)
        return true;
    //attempt to read master key from file using password. return false if password is wrong or not set
    if(cryptoInterface_.readMasterKey(password,QString("./enc/masterkey"))!=CryptoInterface::SUCCESS)
        return false;

    //if successful set encryption boolean to true, load encrypted categories
    encryptionEnabled_=true;
    loadEncryptedCategories();
    return true;
}

void NotesInternals::disableEncryption()
{
    //remove encrypted categories and set encryption boolean to false
    removeEncryptedCategories();
    cryptoInterface_.unsetMasterKey();
    encryptionEnabled_=false;
}

bool NotesInternals::createNewMasterKey(const QCA::SecureArray &newPassword)
{
    //create new master key and save it with newPassword
    cryptoInterface_.setRandomMasterKey();
    if(cryptoInterface_.saveMasterKey(newPassword,"./enc/masterkey")!=CryptoInterface::SUCCESS)
        return false;

    //if encryption is enabled, save all files with new master key and delete the old ones
    if(encryptionEnabled_)
    {
        for(CategorySet::iterator i=categorySet_.begin();i!=categorySet_.end();++i)
        {
            updateCategoryFile(*i);
            for(EntrySet::iterator j=getContent(*i)->entrySet_.begin();j!=getContent(*i)->entrySet_.end();++j)
                updateEntryFile(*i,*j);
        }
    }

    return true;
}

bool NotesInternals::createNewPassword(const QCA::SecureArray &newPassword)
{
    //save existing master key with newPassword
    if(cryptoInterface_.masterKeySet()==false)
        return false;
    if(cryptoInterface_.saveMasterKey(newPassword,"./enc/masterkey")!=CryptoInterface::SUCCESS)
        return false;
    return true;
}

//category and entry selection routines notify UI of changes if a
//tracked ("current") category / entry is affected

void NotesInternals::selectCategory(const Category &category)
{
    if(!isValid(category))
    {
        //if invalid category is selected, set currently selected items to invalid prototypes
        currentCategory_=invalidCategory();
        currentEntry_=invalidEntry();
        //notify UI of changes
        emit categorySelectionChanged();
        emit entrySelectionChanged();
    }
    else
    {
        //if different from the old one, select new category (might have identical content, e.g. renamed), notify UI
        if(currentCategory_!=category)
        {
            currentCategory_=category;
            emit categorySelectionChanged();
            //if currently tracked entry does not belong to selected category anymore, select invalid entry and notify UI
            if(!isValid(currentCategory_,currentEntry_))
            {
                currentEntry_=invalidEntry();
                emit entrySelectionChanged();
            }
        }
    }
}

void NotesInternals::selectEntry(const Entry &entry)
{
    //if entry to be selected is not present in currently selected category, set it to invalid and notify UI
    if(!isValid(currentCategory_,entry))
    {
        currentEntry_=invalidEntry();
        emit entrySelectionChanged();
    }
    else
    {
        //if a new entry is selected, notify UI
        if(currentEntry_!=entry)
        {
            currentEntry_=entry;
            emit entrySelectionChanged();
        }
    }
}

//load either encrypted or plaintext categories and entries from files
void NotesInternals::loadCategories(bool encrypted)
{
    //iterate over encrypted / plaintext folder depending on boolean passed
    QDirIterator dirIterator(encrypted?QString("./enc/"):QString("./plain/"),QDirIterator::Subdirectories);

    //file object to read category and entry files
    QFile file;


    QByteArray content;
    QCA::SecureArray securecontent;

    //folderName holds name of category folders, filePath holds full path (relative to application path) of entries
    QString folderName,filePath;

    //content of categories and entries to store
    QString categoryName;
    QString categoryDateTime;
    QString entryName;
    QString entryDateTime;
    QCA::SecureArray entryText;

    //category and entry pointers to create and temporarily point to new objects
    std::shared_ptr<CategoryContent> categoryContent;
    std::shared_ptr<EntryContent> entryContent;

    //initialization vector in case of encryption. located at start of the file
    QCA::InitializationVector iv;

    //integers to store line break positions
    int i,j;

    //iterate over all subdirectories
    while(dirIterator.hasNext())
    {
        //set folderName to next available subdirectory
        folderName=dirIterator.next();
        //disregard if folder is . or ..
        if(!folderName.endsWith('.'))
        {
            //check for category file in folder
            file.setFileName(folderName+"/category");
            if(file.exists())
            {
                //read category file
                file.open(QFile::ReadOnly);
                //in case of encryption, read initialization vector
                if(encrypted)
                    iv=file.read(32);
                //read rest of content
                content=file.readAll();
                file.close();

                //decrypt content if necessary
                if(encrypted)
                {
                    securecontent=cryptoInterface_.decrypt(QCA::SecureArray(content),iv);
                    content=QByteArray(securecontent.constData(),securecontent.size());
                }

                //find line break, set categoryName to first line
                i=content.indexOf('\n');
                categoryName=content.mid(0,i);
                //set categoryDateTime to second line if possible
                if(i!=-1)
                    categoryDateTime=content.mid(i+1);
                else
                    categoryDateTime="";

                //create new category object corresponding to current folder
                categoryContent=std::make_shared<CategoryContent> ();
                //set content accordingly
                categoryContent->path_=folderName+QString("/");
                categoryContent->encrypted_=encrypted;
                //add newly created category to categorySet_
                categorySet_.insert(Category(categoryName,QDateTime::fromString(categoryDateTime),categoryContent));

                //iterate over *.entry files in current folder
                QDirIterator fileIterator(folderName,QStringList()<<"*.entry",QDir::Files,QDirIterator::NoIteratorFlags);
                while(fileIterator.hasNext())
                {
                    //read entry file
                    filePath=fileIterator.next();
                    file.setFileName(filePath);
                    file.open(QFile::ReadOnly);
                    //read iv if encrypted
                    if(encrypted)
                        iv=file.read(32);
                    //read rest
                    content=file.readAll();
                    file.close();

                    //decrypt if necessary
                    if(encrypted)
                    {
                        securecontent=cryptoInterface_.decrypt(QCA::SecureArray(content),iv);
                        content=QByteArray(securecontent.constData(),securecontent.size());
                    }

                    //find first and second newline. first line is entry name, second is creation date, everything from the 3rd line on is entry text
                    QByteArray content1;
                    QByteArray content2;
                    i=content.indexOf('\n');
                    entryName=content.mid(0,i);
                    content1=QByteArray(content.constData()+i+1);
                    j=content1.indexOf('\n');
                    entryDateTime=content1.mid(0,j);
                    content2=QByteArray(content1.constData()+j+1);
                    entryText=QCA::SecureArray(content2);
                    //create new entry object corresponding to current file
                    entryContent=std::make_shared<EntryContent>();
                    //set content accordingly, remove path of subdirectory from filePath to obtain fileName_
                    entryContent->fileName_=filePath.remove(2,folderName.length()-2);
                    entryContent->text_=entryText;

                    //add entry to current category
                    categoryContent->entrySet_.insert(Entry(entryName,QDateTime::fromString(entryDateTime),entryContent));
                }
            }
        }
    }
    //notify UI of newly added categories
    emit categoryListChanged();
}

void NotesInternals::loadUnencryptedCategories()
{
    loadCategories(false);
}

void NotesInternals::loadEncryptedCategories()
{
    loadCategories(true);
}

void NotesInternals::removeEncryptedCategories()
{
    //iterate over categories
    CategorySet::iterator i=categorySet_.begin();
    while(i!=categorySet_.end())
    {
        if(isEncrypted(*i))
        {
            //if category to be removed is currently selected, select invalid
            if(currentCategory_==*i)
                selectCategory(invalidCategory());
            //advance iterator and remove category from set in one step
            i=categorySet_.erase(i);
        }
        else
            ++i; //just advance iterator otherwise
    }
    //notify UI of removed categories
    emit categoryListChanged();
}

bool NotesInternals::updateEntryFile(Category category, Entry entry)
{
    //save previous file name
    QString previousFileName=getFileName(entry);

    //prepare plaintext content
    QByteArray content;
    QCA::SecureArray plain=QCA::SecureArray(entry.name.toUtf8())
                            +=QCA::SecureArray(QString("\n").toUtf8())
                            +=QCA::SecureArray(entry.date.toString().toUtf8())
                            +=QCA::SecureArray(QString("\n").toUtf8())
                            +=getText(entry);
    //encrypt content if category is set to encrypted, otherwise use plaintext
    if(isEncrypted(category))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoInterface_.encrypt(plain,iv).toByteArray();
    }
    else
        content=plain.toByteArray();

    //backup previous file if there is one
    QFile file;
    if(previousFileName!="")
    {
        file.setFileName(getPath(category)+previousFileName);
        file.rename(getPath(category)+previousFileName+".bak");
    }

    //use hash of content as new filename (with failsafe in case of collision)
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray().toHex());
    QString fileName;
    QRegularExpression regExp("[^A-Za-z0-9]");
    QString entryName=entry.name; //replace has no const qualifier, therefore introduce temporary non-const string
    if(isEncrypted(category))
        fileName=hash+QString(".entry");
    else
        fileName=entryName.replace(regExp,"").left(27)+"-"+hash.left(5)+".entry";
    file.setFileName(getPath(category)+fileName);
    int i=0;
    while(file.exists())
    {
        if(isEncrypted(category))
            fileName=hash+QString("-")+QString::number(i++)+QString(".entry");
        else
            fileName=entryName.replace(regExp,"").left(27)+"-"+hash.left(5)+QString("-")+QString::number(i++)+".entry";
        file.setFileName(getPath(category)+fileName);
    }

    //write content
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();

    //set new filename
    entry.content->fileName_=fileName;

    //delete backup
    if(previousFileName!="")
    {
        file.setFileName(getPath(category)+previousFileName+".bak");
        file.remove();
    }
    return true;
}

bool NotesInternals::updateCategoryFile(Category category)
{
    //save previous folder name
    QString previousFolderName=getPath(category);

    //prepare plaintext content
    QByteArray content;
    QByteArray plain=(category.name+QString("\n")+category.date.toString()).toUtf8();
    if(isEncrypted(category))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoInterface_.encrypt(QCA::SecureArray(plain),iv).toByteArray();
    }
    else
        content=plain;

    //back up existing file, rename previous folder
    if(previousFolderName!="")
    {
        QFile::rename(previousFolderName+"category",previousFolderName+"category.bak");
        //remove '/' in the end of folder name
        previousFolderName=previousFolderName.remove(previousFolderName.size()-1,1);
        QDir("./").rename(previousFolderName,previousFolderName+".tmp");
    }

    //use hash of content for new folder name, again with failsafe for collisions
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray().toHex());
    //pick ./enc or ./plain subdirectories depending on encryption boolean
    QString pre=isEncrypted(category)?QString("./enc/"):QString("./plain/");

    QString folderName;
    QRegularExpression regExp("[^A-Za-z0-9]");
    if(isEncrypted(category))
        folderName=pre+hash+QString("/");
    else
        folderName=pre+category.name.replace(regExp,"").left(27)+"-"+hash.left(5)+QString("/");

    QDir dir(folderName);
    int i=0;
    while(dir.exists())
    {
        if(isEncrypted(category))
            folderName=pre+hash+QString("-")+QString::number(i++)+QString("/");
        else
            folderName=pre+category.name.replace(regExp,"").left(27)+"-"+hash.left(5)+QString("-")+QString::number(i++)+QString("/");
        dir.setPath(folderName);
    }

    //rename previous folder if there was one or create new one
    if(previousFolderName!="")
        QDir("./").rename(previousFolderName+".tmp",folderName);
    else
        QDir("./").mkpath(folderName);

    //write content to category file
    QFile file(folderName+"category");
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();

    //set new path
    category.content->path_=folderName;

    //delete backup
    if(previousFolderName!="")
    {
        file.setFileName(folderName+"category.bak");
        file.remove();
    }
    return true;
}

CategoryContent::CategoryContent()
{

}

CategoryContent::~CategoryContent()
{
    //clean up, i.e. remove entries belonging to category
    entrySet_.clear();
    //for(EntrySet::iterator i=entrySet_.begin();i!=entrySet_.end();++i)
    //    delete NotesInternals::getContent(*i);
}
