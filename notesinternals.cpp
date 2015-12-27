#include "notesinternals.h"



NotesInternals::NotesInternals(QObject *parent) : QObject(parent),hashFunction_("sha256"),currentCategoryPair_(invalidCategoryPair()),currentEntryPair_(invalidEntryPair()),encryptionEnabled_(false)
{
    QCA::init();
    loadUnencryptedCategories();
}

const CategoryPair NotesInternals::addCategory(QString categoryName)
{
    //create category object
    Category* category=new Category();
    //create pair from name, current time, category. insert pair into categorySet_
    CategoryPair ret=CategoryPair(NameDate(categoryName,QDateTime::currentDateTime()),category);
    categorySet_.insert(ret);

    //set encryption according to current encryption state
    category->encrypted_=encryptionEnabled_;

    //set empty folder name, update to create new category folder and file
    category->folderName_="";
    updateCategoryFile(ret);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //return new category pair
    return ret;
}

bool NotesInternals::removeCategory(CategoryPair &categoryPair)
{
    //return false if pair is invalid
    if(!isValid(categoryPair))
        return false;

    //remove category directory and files therein
    QDir dir(getCategoryFolderName(categoryPair));
    dir.removeRecursively();

    //delete category object. entries contained in category will be cleaned up by destructor
    delete getCategory(categoryPair);

    //remove pair from set
    categorySet_.erase(categoryPair);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //if the deleted pair was selected, select invalid pair instead
    if(currentCategoryPair_==categoryPair)
        selectCategory(invalidCategoryPair());

    return true;
}

const CategoryPair NotesInternals::renameCategory(CategoryPair &categoryPair, QString newCategoryName)
{
    //return invalid pair if passed pair is invalid
    if(!isValid(categoryPair))
        return invalidCategoryPair();

    //store category pointer
    Category* category=getCategory_(categoryPair);
    //remove category pair from set
    categorySet_.erase(categoryPair);

    //create new pair with newCategoryName and category pointer, insert into category set
    CategoryPair ret=CategoryPair(NameDate(newCategoryName,getCategoryDate(categoryPair)),category);
    categorySet_.insert(ret);

    //update category file and folder
    updateCategoryFile(ret);

    //set of categories has changed, notify UI
    emit categoryListChanged();

    //if the renamed category was selected, select the newly created pair to retain selection of the same category
    if(currentCategoryPair_==categoryPair)
        selectCategory(ret);

    return ret;
}

const EntryPair NotesInternals::addEntry(CategoryPair &categoryPair, QString entryName)
{
    //return invalid pair if passed category pair is invalid
    if(!isValid(categoryPair))
        return invalidEntryPair();

    //create entry objec
    Entry* entry=new Entry();
    //create pair from name, current time, entry. insert entry into category corresponding to categoryPair
    EntryPair ret=EntryPair(NameDate(entryName,QDateTime::currentDateTime()),entry);
    getCategory_(categoryPair)->entrySet_.insert(ret);

    //set empty file name, update to create new entry file
    entry->fileName_="";
    updateEntryFile(categoryPair,ret);

    //if categoryPair is currently selected, notify GUI of changes
    if(currentCategoryPair_==categoryPair)
        emit categoryContentChanged();
    return ret;
}

bool NotesInternals::removeEntry(CategoryPair &categoryPair, EntryPair &entryPair)
{
    //return false if (categoryPair,entryPair) don't represent a valid entry
    if(!isValid(categoryPair,entryPair))
        return false;

    //remove entry file
    QFile file(getCategoryFolderName(categoryPair)+getEntryFileName(entryPair));
    file.remove();

    //delete entry object
    delete getEntry_(entryPair);
    //remove entry pair from category
    getCategory_(categoryPair)->entrySet_.erase(entryPair);

    //if categoryPair is currently selected, notify GUI
    if(currentCategoryPair_==categoryPair)
    {
        emit categoryContentChanged();
        //if entryPair was currently selected, select invalid pair instead
        if(currentEntryPair_==entryPair)
            selectEntry(invalidEntryPair());
    }
    return true;
}

const EntryPair NotesInternals::renameEntry(CategoryPair &categoryPair, EntryPair &entryPair, QString newEntryName)
{
    //return false if (categoryPair,entryPair) don't represent a valid entry
    if(!isValid(categoryPair,entryPair))
        return invalidEntryPair();

    //remove entry pair from category
    Entry* entry=getEntry_(entryPair);
    Category* category=getCategory_(categoryPair);
    category->entrySet_.erase(entryPair);
    //create entry pair with new name, same entry pointer and creation date, place it into the entry set of the category
    EntryPair ret=EntryPair(NameDate(newEntryName,getEntryDate(entryPair)),entry);
    category->entrySet_.insert(ret);

    //update entry file to reflect name change
    updateEntryFile(categoryPair,ret);

    //if categoryPair is currently selected, notify UI
    if(currentCategoryPair_==categoryPair)
    {
        emit categoryContentChanged();
        //if entry pair was currently selected, select the newly created one
        if(currentEntryPair_==entryPair)
            selectEntry(ret);
    }

    //return entry pair with new name
    return ret;
}

const EntryPair NotesInternals::moveEntry(CategoryPair &oldCategoryPair, EntryPair &entryPair, CategoryPair &newCategoryPair)
{
    //if either (oldCategoryPair,entryPair) don't represent a valid entry or newCategoryPair does not represent a category, return invalid pair
    if(!isValid(oldCategoryPair,entryPair) || !isValid(newCategoryPair))
        return invalidEntryPair();

    //remove entry from old category, insert into new one
    getCategory_(oldCategoryPair)->entrySet_.erase(entryPair);
    getCategory_(newCategoryPair)->entrySet_.insert(entryPair);

    //rename entry file, move it to new category folder, update fileName_
    QDir("./").rename(getCategoryFolderName(oldCategoryPair)+getEntry_(entryPair)->fileName_,getCategoryFolderName(newCategoryPair)+getEntry_(entryPair)->fileName_+".tmp");
    getEntry_(entryPair)->fileName_=getEntry_(entryPair)->fileName_+".tmp";

    //update entry file, which means delete the moved and renamed version, create a new one
    //(deals with both possible existing file of the same name as well as the case where one category is encrypted and the other is not)
    updateEntryFile(newCategoryPair,entryPair);

    //if currently selected category is either the previous or new category of the moved entry, notify UI of changes
    if(currentCategoryPair_==oldCategoryPair || currentCategoryPair_==newCategoryPair)
    {
        emit categoryContentChanged();
        //if entry was currently selected, select invalid pair instead
        if(currentCategoryPair_==oldCategoryPair && currentEntryPair_==entryPair)
            selectEntry(invalidEntryPair());
    }

    //return entry pair passed (no internal changes have occured)
    return entryPair;
}

const EntryPair NotesInternals::modifyEntryText(CategoryPair categoryPair, EntryPair entryPair, QCA::SecureArray newEntryText)
{
    //return invalid entry pair in case (categoryPair,entryPair) does not represent a valid entry
    if(!isValid(categoryPair,entryPair))
        return invalidEntryPair();

    //update entry text
    getEntry_(entryPair)->entryText_=newEntryText;

    //update entry file with new text
    updateEntryFile(categoryPair,entryPair);

    //notify UI if entry is currently selected
    if(currentCategoryPair_==categoryPair && currentEntryPair_==entryPair)
        emit entryContentChanged();

    //return entry pair (above changes do not affect the pair directly)
    return entryPair;
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
            for(EntrySet::iterator j=getCategory(*i)->entrySet_.begin();j!=getCategory(*i)->entrySet_.end();++j)
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

//category and entry selection routines notify UI of changes if either a tracked pair is or becomes invalid
//or the newly selected pair is different from the previous one

void NotesInternals::selectCategory(const CategoryPair &categoryPair)
{
    if(!isValid(categoryPair))
    {
        //if invalid category is selected, set currently selected pairs to invalid prototypes
        currentCategoryPair_=invalidCategoryPair();
        currentEntryPair_=invalidEntryPair();
        //notify UI of changes
        emit categorySelectionChanged();
        emit entrySelectionChanged();
    }
    else
    {
        //if different from the old one, select new category pair (might belong to same category, e.g. renamed), notify UI
        if(currentCategoryPair_!=categoryPair)
        {
            currentCategoryPair_=categoryPair;
            emit categorySelectionChanged();
            //if currently tracked entry does not belong to selected category anymore, select invalid entry and notify UI
            if(!isValid(currentCategoryPair_,currentEntryPair_))
            {
                currentEntryPair_=invalidEntryPair();
                emit entrySelectionChanged();
            }
        }
    }
}

void NotesInternals::selectEntry(const EntryPair &entryPair)
{
    //if entry to be selected is not present in currently selected category, set it to invalid and notify UI
    if(!isValid(currentCategoryPair_,entryPair))
    {
        currentEntryPair_=invalidEntryPair();
        emit entrySelectionChanged();
    }
    else
    {
        //if a new entry pair is selected, notify UI
        if(currentEntryPair_!=entryPair)
        {
            currentEntryPair_=entryPair;
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
    Category *category;
    Entry *entry;

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
                category=new Category();
                //set content accordingly
                category->folderName_=folderName+QString("/");
                category->encrypted_=encrypted;
                //add category pair for newly created category to categorySet_
                categorySet_.insert(CategoryPair(NameDate(categoryName,QDateTime::fromString(categoryDateTime)),category));

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
                    entry=new Entry();
                    //set content accordingly, remove path of subdirectory from filePath to obtain fileName_
                    entry->fileName_=filePath.remove(2,folderName.length()-2);
                    entry->entryText_=entryText;

                    //add entry pair to current category
                    category->entrySet_.insert(EntryPair(NameDate(entryName,QDateTime::fromString(entryDateTime)),entry));
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
        //if category is encrypted, delete category object, remove pair from categorySet_
        if(getCategoryEncrypted(*i))
        {
            //if category pair to be removed is currently selected, select invalid pair
            if(currentCategoryPair_==*i)
                selectCategory(invalidCategoryPair());
            delete getCategory(*i);
            //advance iterator and remove pair from categorySet_ in one step
            i=categorySet_.erase(i);
        }
        else
            ++i; //just advance iterator otherwise
    }
    //notify UI of removed categories
    emit categoryListChanged();
}

bool NotesInternals::updateEntryFile(CategoryPair categoryPair, EntryPair entryPair)
{
    //save previous file name
    QString previousFileName=getEntryFileName(entryPair);

    //prepare plaintext content
    QByteArray content;
    QCA::SecureArray plain=QCA::SecureArray(getEntryName(entryPair).toUtf8())
                            +=QCA::SecureArray(QString("\n").toUtf8())
                            +=QCA::SecureArray(getEntryDate(entryPair).toString().toUtf8())
                            +=QCA::SecureArray(QString("\n").toUtf8())
                            +=getEntryText(entryPair);
    //encrypt content if category is set to encrypted, otherwise use plaintext
    if(getCategoryEncrypted(categoryPair))
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
        file.setFileName(getCategoryFolderName(categoryPair)+previousFileName);
        file.rename(getCategoryFolderName(categoryPair)+previousFileName+".bak");
    }

    //use hash of content as new filename (with failsafe in case of collision)
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray().toHex());
    QString fileName=hash+QString(".entry");
    file.setFileName(getCategoryFolderName(categoryPair)+fileName);
    int i=0;
    while(file.exists())
    {
        fileName=hash+QString("-")+QString::number(i++)+QString(".entry");
        file.setFileName(getCategoryFolderName(categoryPair)+fileName);
    }

    //write content
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();

    //set new filename
    getEntry_(entryPair)->fileName_=fileName;

    //delete backup
    if(previousFileName!="")
    {
        file.setFileName(getCategoryFolderName(categoryPair)+previousFileName+".bak");
        file.remove();
    }
    return true;
}

bool NotesInternals::updateCategoryFile(CategoryPair categoryPair)
{
    //save previous folder name
    QString previousFolderName=getCategoryFolderName(categoryPair);

    //prepare plaintext content
    QByteArray content;
    QByteArray plain=(getCategoryName(categoryPair)+QString("\n")+getCategoryDate(categoryPair).toString()).toUtf8();
    if(getCategoryEncrypted(categoryPair))
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
    QString pre=getCategoryEncrypted(categoryPair)?QString("./enc/"):QString("./plain/");
    QString folderName=pre+hash+QString("/");
    QDir dir(folderName);
    int i=0;
    while(dir.exists())
    {
        folderName=pre+hash+QString("-")+QString::number(i++)+QString("/");
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

    //set new folder name
    getCategory_(categoryPair)->folderName_=folderName;

    //delete backup
    if(previousFolderName!="")
    {
        file.setFileName(folderName+"category.bak");
        file.remove();
    }
    return true;
}

Category::Category()
{

}

Category::~Category()
{
    //clean up, i.e. remove entries belonging to category
    for(EntrySet::iterator i=entrySet_.begin();i!=entrySet_.end();++i)
        delete NotesInternals::getEntry(*i);
}
