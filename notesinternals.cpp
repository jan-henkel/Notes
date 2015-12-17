#include "notesinternals.h"

NotesInternals::NotesInternals(QObject *parent) : QObject(parent),hashFunction_("sha256"),encryptionEnabled_(false)
{
    QCA::init();
    loadUnencryptedCategories();
}

const CategoryPair NotesInternals::addCategory(QString categoryName)
{
    Category* category=new Category();
    CategoryPair ret=CategoryPair(NameDate(categoryName,QDateTime::currentDateTime()),category);
    categoriesMap_.insert(ret);
    category->encrypted_=encryptionEnabled_;
    category->folderName_="";
    updateCategoryFile(ret);
    return ret;
}

bool NotesInternals::removeCategory(CategoryPair &categoryPair)
{
    if(!isValid(categoryPair))
        return false;
    QDir dir(getCategoryFolderName(categoryPair));
    dir.removeRecursively();
    delete getCategory(categoryPair);
    categoriesMap_.erase(categoryPair);
    return true;
}

const CategoryPair NotesInternals::renameCategory(CategoryPair &categoryPair, QString newCategoryName)
{
    if(!isValid(categoryPair))
        return invalidCategoryPair();
    QString oldFolderName=getCategoryFolderName(categoryPair);
    Category* category=getCategory_(categoryPair);
    if(!QDir("./").rename(oldFolderName,oldFolderName+".bak"))
        return invalidCategoryPair();
    categoriesMap_.erase(categoryPair);
    CategoryPair ret=CategoryPair(NameDate(newCategoryName,QDateTime::currentDateTime()),category);
    categoriesMap_.insert(ret);
    updateCategoryFile(ret);
    return ret;
}

const EntryPair NotesInternals::addEntry(CategoryPair &categoryPair, QString entryName)
{
    if(!isValid(categoryPair))
        return invalidEntryPair();
    Entry* entry=new Entry();
    EntryPair ret=EntryPair(NameDate(entryName,QDateTime::currentDateTime()),entry);
    getCategory_(categoryPair)->entriesMap_.insert(ret);
    entry->fileName_="";
    updateEntryFile(categoryPair,ret);
    return ret;
}

bool NotesInternals::removeEntry(CategoryPair &categoryPair, EntryPair &entryPair)
{
    if(!isValid(categoryPair,entryPair))
        return false;
    QFile file(getCategoryFolderName(categoryPair)+getEntryFileName(entryPair));
    file.remove();
    delete getEntry_(entryPair);
    getCategory_(categoryPair)->entriesMap_.erase(entryPair);
    return true;
}

const EntryPair NotesInternals::renameEntry(CategoryPair &categoryPair, EntryPair &entryPair, QString newEntryName)
{
    if(!isValid(categoryPair,entryPair))
        return invalidEntryPair();
    QString oldFileName=getCategoryFolderName(categoryPair)+getEntryFileName(entryPair);
    Entry* entry=getEntry_(entryPair);
    Category* category=getCategory_(categoryPair);
    QFile file(oldFileName);
    if(!file.rename(oldFileName+".bak"))
        return invalidEntryPair();
    category->entriesMap_.erase(entryPair);
    EntryPair ret=EntryPair(NameDate(newEntryName,QDateTime::currentDateTime()),entry);
    category->entriesMap_.insert(ret);
    updateEntryFile(categoryPair,ret);
    return ret;
}

const EntryPair NotesInternals::moveEntry(CategoryPair &oldCategoryPair, EntryPair &entryPair, CategoryPair &newCategoryPair)
{
    if(!isValid(oldCategoryPair,entryPair) || !isValid(newCategoryPair))
        return invalidEntryPair();
    getCategory_(oldCategoryPair)->entriesMap_.erase(entryPair);
    getCategory_(newCategoryPair)->entriesMap_.insert(entryPair);
    QDir("./").rename(getCategoryFolderName(oldCategoryPair)+getEntry_(entryPair)->fileName_,getCategoryFolderName(newCategoryPair)+getEntry_(entryPair)->fileName_+".tmp");
    getEntry_(entryPair)->fileName_=getEntry_(entryPair)->fileName_+".tmp";
    updateEntryFile(newCategoryPair,entryPair);
    return entryPair;
}

const EntryPair NotesInternals::modifyEntryText(CategoryPair &categoryPair, EntryPair &entryPair, QString newEntryText)
{
    if(!isValid(categoryPair,entryPair))
        return invalidEntryPair();
    getEntry_(entryPair)->entryText_=newEntryText;
    updateEntryFile(categoryPair,entryPair);
    return entryPair;
}

bool NotesInternals::enableEncryption(const QCA::SecureArray &password)
{
    if(!cryptoBuffer_.readKeyIV(password,QString("./enc/pw")))
        return false;
    encryptionEnabled_=true;
    loadEncryptedCategories();
    return true;
}

void NotesInternals::disableEncryption()
{
    removeEncryptedCategories();
    encryptionEnabled_=false;
}

void NotesInternals::loadCategories(bool encrypted)
{
    QDirIterator dirIterator(encrypted?QString("./enc/"):QString("./plain/"),QDirIterator::Subdirectories);
    QFile file;
    QByteArray content;
    QString folderName,filePath;
    QString categoryName;
    QString entryName;
    QString entryDateTime;
    QString entryText;
    Category *category;
    Entry *entry;
    QCA::InitializationVector iv;
    while(dirIterator.hasNext())
    {
        folderName=dirIterator.next();
        if(!folderName.endsWith('.'))
        {
            file.setFileName(folderName+"/category");
            if(file.exists())
            {
                //read category file
                file.open(QFile::ReadOnly);
                if(encrypted)
                    iv=file.read(32);
                content=file.readAll();
                file.close();

                if(encrypted)
                    content=cryptoBuffer_.decrypt(QCA::SecureArray(content),iv).toByteArray();

                categoryName=(content.indexOf('\n')==-1)?content:content.mid(0,content.indexOf('\n'));

                //create new category from current folder
                category=new Category();
                category->folderName_=folderName+QString("/");
                category->encrypted_=false;

                categoriesMap_.insert(CategoryPair(NameDate(categoryName,QDateTime::currentDateTime()),category));

                QDirIterator fileIterator(folderName,QStringList()<<"*.entry",QDir::Files,QDirIterator::NoIteratorFlags);

                while(fileIterator.hasNext())
                {
                    filePath=fileIterator.next();
                    file.setFileName(filePath);
                    file.open(QFile::ReadOnly);
                    if(encrypted)
                        iv=file.read(32);
                    content=file.readAll();
                    file.close();

                    if(encrypted)
                        content=cryptoBuffer_.decrypt(QCA::SecureArray(content),iv).toByteArray();

                    int i=content.indexOf('\n');
                    entryName=content.mid(0,i);
                    int j=content.mid(i+1).indexOf('\n');
                    entryDateTime=content.mid(i+1,j);
                    entryText=content.mid(i+j+2);

                    entry=new Entry();
                    entry->fileName_=filePath.remove(2,folderName.length()-2);
                    entry->entryText_=entryText;

                    category->entriesMap_.insert(EntryPair(NameDate(entryName,QDateTime::fromString(entryDateTime)),entry));
                }
            }
        }
    }
}

void NotesInternals::loadUnencryptedCategories()
{
    loadCategories(false);
  /*  QDirIterator dirIterator("./plain/",QDirIterator::Subdirectories);
    QFile file;
    QByteArray content;
    QString folderName,filePath;
    QString categoryName;
    QString entryName;
    QString entryText;
    Category *category;
    Entry *entry;
    while(dirIterator.hasNext())
    {
        folderName=dirIterator.next();
        if(!folderName.endsWith('.'))
        {
            file.setFileName(folderName+"/category");
            if(file.exists())
            {
                //read category file
                file.open(QFile::ReadOnly);
                content=file.readAll();
                file.close();

                categoryName=(content.indexOf('\n')==-1)?content:content.mid(0,content.indexOf('\n'));

                //create new category from current folder
                category=new Category();
                category->folderName_=folderName+QString("/");
                category->encrypted_=false;

                categoriesMap_.insert(std::pair<NameDate,Category*>(NameDate(categoryName,QDateTime::currentDateTime()),category));

                QDirIterator fileIterator(folderName,QStringList()<<"*.entry",QDir::Files,QDirIterator::NoIteratorFlags);

                while(fileIterator.hasNext())
                {
                    filePath=fileIterator.next();
                    file.setFileName(filePath);
                    file.open(QFile::ReadOnly);
                    content=file.readAll();
                    file.close();


                    int i=content.indexOf('\n');
                    entryName=content.mid(0,i);
                    entryText=content.mid(i+1);

                    entry=new Entry();
                    entry->fileName_=filePath.remove(2,folderName.length()-2);
                    entry->entryText_=entryText;

                    category->entriesMap_.insert(std::pair<NameDate,Entry*>(NameDate(entryName,QDateTime::currentDateTime()),entry));
                }
            }
        }
    }*/
}

void NotesInternals::loadEncryptedCategories()
{
    loadCategories(true);
    /*
    QDirIterator dirIterator("./plain/",QDirIterator::Subdirectories);
    QFile file;
    QCA::InitializationVector iv;
    QByteArray cipher;
    QByteArray content;
    QString folderName,filePath;
    QString categoryName;
    QString entryName;
    QString entryText;
    Category *category;
    Entry *entry;
    while(dirIterator.hasNext())
    {
        folderName=dirIterator.next();
        if(!folderName.endsWith('.'))
        {
            file.setFileName(folderName+"/category");
            if(file.exists())
            {
                //read category file
                file.open(QFile::ReadOnly);
                iv=file.read(32);
                cipher=file.readAll();
                file.close();

                content=cryptoBuffer_.decrypt(QCA::SecureArray(cipher),iv).toByteArray();

                categoryName=(content.indexOf('\n')==-1)?content:content.mid(0,content.indexOf('\n'));

                //create new category from current folder
                category=new Category();
                category->folderName_=folderName+QString("/");
                category->encrypted_=true;

                categoriesMap_.insert(std::pair<NameDate,Category*>(NameDate(categoryName,QDateTime::currentDateTime()),category));

                QDirIterator fileIterator(folderName,QStringList()<<"*.entry",QDir::Files,QDirIterator::NoIteratorFlags);

                while(fileIterator.hasNext())
                {
                    filePath=fileIterator.next();
                    file.setFileName(filePath);
                    file.open(QFile::ReadOnly);
                    iv=file.read(32);
                    cipher=file.readAll();
                    file.close();

                    content=cryptoBuffer_.decrypt(QCA::SecureArray(cipher),iv).toByteArray();

                    int i=content.indexOf('\n');
                    entryName=content.mid(0,i);
                    entryText=content.mid(i+1);

                    entry=new Entry();
                    entry->fileName_=filePath.remove(2,folderName.length()-2);
                    entry->entryText_=entryText;

                    category->entriesMap_.insert(std::pair<NameDate,Entry*>(NameDate(entryName,QDateTime::currentDateTime()),entry));
                }
            }
        }
    }*/
}

void NotesInternals::removeEncryptedCategories()
{
    CategoriesMap::iterator i=categoriesMap_.begin();
    while(i!=categoriesMap_.end())
    {
        if(getCategoryEncrypted(*i))
        {
            delete getCategory(*i);
            i=categoriesMap_.erase(i);
        }
        else
            ++i;
    }
}

bool NotesInternals::updateEntryFile(CategoryPair &categoryPair, EntryPair &entryPair)
{
    //save previous file name
    QString previousFileName=getEntryFileName(entryPair);

    //prepare content to write, depending on encryption
    QByteArray content;
    QByteArray plain=(getEntryName(entryPair)+QString("\n")+getEntryDate(entryPair)+QString("\n")+getEntryText(entryPair)).toUtf8();
    if(getCategoryEncrypted(categoryPair))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoBuffer_.encrypt(QCA::SecureArray(plain),iv).toByteArray();
    }
    else
        content=plain;

    //backup previous file if there is one
    QFile file;
    if(previousFileName!="")
    {
        file.setFileName(getCategoryFolderName(categoryPair)+previousFileName);
        file.rename(previousFileName+".bak");
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
        file.setFileName(getCategoryFolderName(categoryPair)+previousFileName);
        file.remove();
    }
    return true;
}

bool NotesInternals::updateCategoryFile(CategoryPair &categoryPair)
{
    //save previous folder name
    QString previousFolderName=getCategoryFolderName(categoryPair);

    //prepare content
    QByteArray content;
    if(getCategoryEncrypted(categoryPair))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoBuffer_.encrypt(QCA::SecureArray(getCategoryName(categoryPair).toUtf8()),iv).toByteArray();
    }
    else
        content=getCategoryName(categoryPair).toUtf8();

    //back up existing file, rename previous folder
    if(previousFolderName!="")
    {
        QFile::rename(previousFolderName+"category",previousFolderName+"category.bak");
        QDir("./").rename(previousFolderName,previousFolderName+".tmp");
    }

    //use hash of content for new folder name, again with failsafe for collisions
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray().toHex());
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
    for(EntriesMap::iterator i=entriesMap_.begin();i!=entriesMap_.end();++i)
        delete NotesInternals::getEntry(*i);
}
