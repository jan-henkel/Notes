#include "notesinternals.h"

NotesInternals::NotesInternals(QObject *parent) : QObject(parent),hashFunction_("sha256"),encryptionEnabled_(false)
{
    QCA::init();
    loadUnencryptedCategories();
}

const CategoriesMap::iterator NotesInternals::addCategory(QString categoryName)
{
    Category* category=new Category();
    CategoriesMap::iterator ret=categoriesMap_.insert(std::pair<NameDate,Category*>(NameDate(categoryName,QDateTime::currentDateTime()),category));
    category->encrypted_=encryptionEnabled_;
    category->folderName_="";
    updateCategoryFile(ret);
    return ret;
}

void NotesInternals::removeCategory(CategoriesMap::iterator &categoryIterator)
{
    QDir dir(getCategoryFolderName(categoryIterator));
    dir.removeRecursively();
    delete getCategory(categoryIterator);
    categoriesMap_.erase(categoryIterator);
}

const CategoriesMap::iterator NotesInternals::renameCategory(CategoriesMap::iterator &categoryIterator, QString newCategoryName)
{
    QString oldFolderName=getCategoryFolderName(categoryIterator);
    Category* category=getCategory_(categoryIterator);
    if(!QDir("./").rename(oldFolderName,oldFolderName+".bak"))
        return categoriesMap_.end();
    categoriesMap_.erase(categoryIterator);
    CategoriesMap::iterator ret=categoriesMap_.insert(std::pair<NameDate,Category*>(NameDate(newCategoryName,QDateTime::currentDateTime()),category));
    updateCategoryFile(ret);
    return ret;
}

const EntriesMap::iterator NotesInternals::addEntry(CategoriesMap::iterator &categoryIterator, QString entryName)
{
    Entry* entry=new Entry();
    EntriesMap::iterator ret=getCategory_(categoryIterator)->entriesMap_.insert(std::pair<NameDate,Entry*>(NameDate(entryName,QDateTime::currentDateTime()),entry));
    entry->fileName_="";
    updateEntryFile(categoryIterator,ret);
    return ret;
}

void NotesInternals::removeEntry(CategoriesMap::iterator &categoryIterator, EntriesMap::iterator &entryIterator)
{
    QFile file(getCategoryFolderName(categoryIterator)+getEntryFileName(entryIterator));
    file.remove();
    delete getEntry_(entryIterator);
    getCategory_(categoryIterator)->entriesMap_.erase(entryIterator);
}

const EntriesMap::iterator NotesInternals::renameEntry(CategoriesMap::iterator &categoryIterator, EntriesMap::iterator &entryIterator, QString newEntryName)
{
    QString oldFileName=getCategoryFolderName(categoryIterator)+getEntryFileName(entryIterator);
    Entry* entry=getEntry_(entryIterator);
    Category* category=getCategory_(categoryIterator);
    QFile file(oldFileName);
    if(!file.rename(oldFileName+".bak"))
        return category->entriesMap_.end();
    category->entriesMap_.erase(entryIterator);
    EntriesMap::iterator ret=category->entriesMap_.insert(std::pair<NameDate,Entry*>(NameDate(newEntryName,QDateTime::currentDateTime()),entry));
    updateEntryFile(categoryIterator,ret);
    return ret;
}

const EntriesMap::iterator NotesInternals::moveEntry(CategoriesMap::iterator &oldCategoryIterator, EntriesMap::iterator &entryIterator, CategoriesMap::iterator &newCategoryIterator)
{
    std::pair<NameDate,Entry*> tmp=*entryIterator;
    getCategory_(oldCategoryIterator)->entriesMap_.erase(entryIterator);
    EntriesMap::iterator ret=getCategory_(newCategoryIterator)->entriesMap_.insert(tmp);
    QDir("./").rename(getCategoryFolderName(oldCategoryIterator)+getEntry_(ret)->fileName_,getCategoryFolderName(newCategoryIterator)+getEntry_(ret)->fileName_+".tmp");
    getEntry_(ret)->fileName_=getEntry_(ret)->fileName_+".tmp";
    updateEntryFile(newCategoryIterator,ret);
    return ret;
}

const EntriesMap::iterator NotesInternals::modifyEntryText(CategoriesMap::iterator &categoryIterator, EntriesMap::iterator &entryIterator, QString newEntryText)
{
    getEntry_(entryIterator)->entryText_=newEntryText;
    updateEntryFile(categoryIterator,entryIterator);
    return entryIterator;
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

void NotesInternals::loadUnencryptedCategories()
{
    QDirIterator dirIterator("./plain/",QDirIterator::Subdirectories);
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
            file.setFileName(folderName+"category");
            if(file.exists())
            {
                //read category file
                file.open(QFile::ReadOnly);
                content=file.readAll();
                file.close();

                categoryName=(content.indexOf('\n')==-1)?content:content.mid(0,content.indexOf('\n'));

                //create new category from current folder
                category=new Category();
                category->folderName_=folderName;
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
                    entryText=content.mid(i);

                    entry=new Entry();
                    entry->fileName_=QDir(folderName).relativeFilePath(filePath);
                    entry->entryText_=entryText;

                    category->entriesMap_.insert(std::pair<NameDate,Entry*>(NameDate(entryName,QDateTime::currentDateTime()),entry));
                }
            }
        }
    }
}

void NotesInternals::loadEncryptedCategories()
{

}

void NotesInternals::removeEncryptedCategories()
{

}

bool NotesInternals::updateEntryFile(CategoriesMap::iterator &categoryIterator, EntriesMap::iterator &entryIterator)
{
    //save previous file name
    QString previousFileName=getEntryFileName(entryIterator);

    //prepare content to write, depending on encryption
    QByteArray content;
    if(getCategoryEncrypted(categoryIterator))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoBuffer_.encrypt(QCA::SecureArray((getEntryName(entryIterator)+QString("\n")+getEntryText(entryIterator)).toUtf8()),iv).toByteArray();
    }
    else
        content=(getEntryName(entryIterator)+QString("\n")+getEntryText(entryIterator)).toUtf8();

    //backup previous file if there is one
    QFile file;
    if(previousFileName!="")
    {
        file.setFileName(getCategoryFolderName(categoryIterator)+previousFileName);
        file.rename(previousFileName+".bak");
    }

    //use hash of content as new filename (with failsafe in case of collision)
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray());
    QString fileName=hash+QString(".entry");
    file.setFileName(getCategoryFolderName(categoryIterator)+fileName);
    int i=0;
    while(file.exists())
    {
        fileName=hash+QString("-")+QString::number(i++)+QString(".entry");
        file.setFileName(getCategoryFolderName(categoryIterator)+fileName);
    }

    //write content
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();

    //set new filename
    getEntry_(entryIterator)->fileName_=fileName;

    //delete backup
    if(previousFileName!="")
    {
        file.setFileName(getCategoryFolderName(categoryIterator)+previousFileName);
        file.remove();
    }
    return true;
}

bool NotesInternals::updateCategoryFile(CategoriesMap::iterator &categoryIterator)
{
    //save previous folder name
    QString previousFolderName=getCategoryFolderName(categoryIterator);

    //prepare content
    QByteArray content;
    if(getCategoryEncrypted(categoryIterator))
    {
        QCA::InitializationVector iv(32);
        content=iv.toByteArray()+cryptoBuffer_.encrypt(QCA::SecureArray(getCategoryName(categoryIterator).toUtf8()),iv).toByteArray();
    }
    else
        content=getCategoryName(categoryIterator).toUtf8();

    //back up existing file, rename previous folder
    if(previousFolderName!="")
    {
        QFile::rename(previousFolderName+"category",previousFolderName+"category.bak");
        QDir("./").rename(previousFolderName,previousFolderName+".tmp");
    }

    //use hash of content for new folder name, again with failsafe for collisions
    hashFunction_.clear();
    hashFunction_.update(content);
    QString hash=QString(hashFunction_.final().toByteArray());
    QString pre=getCategoryEncrypted(categoryIterator)?QString("./enc/"):QString("./plain/");
    QString folderName=pre+hash+QString("/");
    QDir dir(folderName);
    int i=0;
    while(dir.exists())
    {
        folderName=pre+hash+QString::number(i++)+QString("/");
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
    getCategory_(categoryIterator)->folderName_=folderName;

    //delete backup
    if(previousFolderName!="")
    {
        file.setFileName(folderName+"category.bak");
        file.remove();
    }
    return true;
}

/*
NotesInternals::NotesInternals(QObject *parent) : QObject(parent),hashFunction("sha256"), encryptionActive_(false)
{
    QCA::init();
}

NotesInternals::Result NotesInternals::createCategory(QString categoryName, bool encrypted)
{
    if(categoryExists(categoryName,encrypted))
        return CATEGORY_EXISTS;
    if(encrypted && !encryptionActive_)
        return ENCRYPTION_INACTIVE;
    if(!createCategory(categoryName,encrypted,true))
        return FILE_ERROR;
    return SUCCESS;
}

NotesInternals::Result NotesInternals::createEntry(QString categoryName, QString entryName, QCA::SecureArray entryText, bool encrypted)
{
    if(entryExists(categoryName,entryName,encrypted))
        return ENTRY_EXISTS;
    if(encrypted && !encryptionActive_)
        return ENCRYPTION_INACTIVE;
    if(!categoryExists(categoryName,encrypted))
        createCategory(categoryName,encrypted);
    if(!createEntry(categoryName,entryName,entryText,encrypted,true))
        return FILE_ERROR;
    return SUCCESS;
}

NotesInternals::Result NotesInternals::deleteCategory(QString categoryName, bool encrypted)
{
    if(!categoryExists(categoryName,encrypted))
        return CATEGORY_DOES_NOT_EXIST;
    QFile file;
    Category* category;
    Entry* entry;
    category=getCategory(categoryName,encrypted);
    Category::iterator i;
    for(i=category->begin();i!=category->end();++i)
    {
        entry=(*i).second;
        file.setFileName(entry->fileName_);
        file.remove();
        delete entry;
    }
    delete category;
    return SUCCESS;
}

NotesInternals::Result NotesInternals::deleteEntry(QString categoryName, QString entryName, bool encrypted)
{
    if(!entryExists(categoryName,entryName,encrypted))
        return ENTRY_DOES_NOT_EXIST;
    if(!deleteEntry(categoryName,entryName,encrypted,true))
        return FILE_ERROR;
    return SUCCESS;
}

NotesInternals::Result NotesInternals::renameCategory(QString oldCategoryName, QString newCategoryName, bool encrypted)
{
    if(!categoryExists(oldCategoryName,encrypted))
        return CATEGORY_DOES_NOT_EXIST;
    if(categoryExists(newCategoryName,encrypted))
        return CATEGORY_EXISTS;
    CategoryList *list;
    list=encrypted?(&encryptedCategoryList_):(&unencryptedCategoryList_);
    (*list)[newCategoryName]=(*list)[oldCategoryName];
    list->erase(oldCategoryName);

    return SUCCESS;
}

NotesInternals::Result NotesInternals::renameEntry(QString categoryName, QString oldEntryName, QString newEntryName, bool encrypted)
{
    if(!entryExists(categoryName,oldEntryName,encrypted))
        return ENTRY_DOES_NOT_EXIST;
    if(entryExists(categoryName,newEntryName,encrypted))
        return ENTRY_EXISTS;
    Category* category=getCategory(categoryName,encrypted);
    (*category)[newEntryName]=(*category)[oldEntryName];
    category->erase(oldEntryName);
    return SUCCESS;
}

NotesInternals::Result NotesInternals::changeEntryCategory(QString entryName, QString oldCategoryName, QString newCategoryName, bool encrypted)
{
    if(!entryExists(oldCategoryName,entryName,encrypted))
        return ENTRY_DOES_NOT_EXIST;
    if(entryExists(newCategoryName,entryName,encrypted))
        return ENTRY_EXISTS;
    Category* oldCategory=getCategory(oldCategoryName,encrypted);
    Category* newCategory=getCategory(newCategoryName,encrypted);
    (*newCategory)[entryName]=(*oldCategory)[entryName];
    oldCategory->erase(entryName);
    return SUCCESS;
}

NotesInternals::Result NotesInternals::getCategoryList(bool encrypted, QStringList &rList)
{
    if(encrypted && !encryptionActive_)
        return ENCRYPTION_INACTIVE;
    CategoryList *list;
    list=encrypted?(&encryptedCategoryList_):(&unencryptedCategoryList_);
    for(CategoryList::iterator i=list->begin();i!=list->end();++i)
        rList.push_back((*i).first);
    return SUCCESS;
}

NotesInternals::Result NotesInternals::getCategoryEntries(QString categoryName, bool encrypted, QStringList &rList)
{
    if(!categoryExists(categoryName,encrypted))
        return CATEGORY_DOES_NOT_EXIST;
    Category* category=getCategory(categoryName,encrypted);
    rList.clear();
    for(Category::iterator i=category->begin();i!=category->end();++i)
        rList.push_back((*i).first);
    return SUCCESS;
}

NotesInternals::Result NotesInternals::getEntryText(QString categoryName, QString entryName, bool encrypted, QCA::SecureArray &rText)
{
    if(!entryExists(categoryName,entryName,encrypted))
        return ENTRY_DOES_NOT_EXIST;
    Entry* entry=getEntry(categoryName,entryName,encrypted);
    rText=entry->entryText_;
    return SUCCESS;
}

NotesInternals::Result NotesInternals::toggleCategoryEncryption(QString categoryName, bool encrypted)
{
    if(!categoryExists(categoryName,encrypted))
        return CATEGORY_DOES_NOT_EXIST;
    if(categoryExists(categoryName,!encrypted))
        return CATEGORY_EXISTS;
    CategoryList *oldlist,*newlist;
    oldlist=encrypted?(&encryptedCategoryList_):(&unencryptedCategoryList_);
    newlist=(!encrypted)?(&encryptedCategoryList_):(&unencryptedCategoryList_);
    (*newlist)[categoryName]=(*oldlist)[categoryName];
    (*oldlist).erase(categoryName);
    return SUCCESS;
}

void NotesInternals::hash256(const QCA::SecureArray &data, QByteArray &rHash)
{
    hashFunction.clear();
    hashFunction.update(data);
    rHash=hashFunction.final().toByteArray();
}

QByteArray NotesInternals::hash256(const QCA::SecureArray &data)
{
    QByteArray ret;
    hash256(data,ret);
    return ret;
}

bool NotesInternals::createEntry(CategoryList::iterator categoryIterator, QString entryName, const QCA::SecureArray &entryText, bool encrypted, bool saveFile)
{
    Category *category=(*categoryIterator).second;
    //Create new entry, set text
    Entry* entry=new Entry();
    entry->entryText_=entryText;
    //Add entry to category
    Category::iterator entryIterator=(*category).insert(std::pair<QString,Entry*>(entryName,entry)).first;
    if(saveFile)
        return createEntryFile((*categoryIterator).first,entryIterator,encrypted);
    return true;
}

bool NotesInternals::createEntry(QString categoryName, QString entryName, const QCA::SecureArray &entryText, bool encrypted, bool saveFile)
{
    return createEntry(getCategoryList(encrypted)->find(categoryName),entryName,entryText,encrypted,saveFile);
}

bool NotesInternals::deleteEntry(CategoryList::iterator categoryIterator, Category::iterator entryIterator, bool encrypted, bool removeFile)
{
    (*categoryIterator).second->erase(entryIterator);
    if(removeFile)
    {
        QFile file(getCategoryFolderName((*categoryIterator).first,encrypted)+(*entryIterator).second->fileName_);
        if(!file.remove())
            return false;
    }
    delete (*entryIterator).second;
}

bool NotesInternals::deleteEntry(QString categoryName, QString entryName, bool encrypted, bool removeFile)
{
    CategoryList::iterator ci=getCategoryList(encrypted)->find(categoryName);
    return deleteEntry(ci,((*ci).second)[entryName],encrypted,removeFile);
}

bool NotesInternals::createCategory(QString categoryName, bool encrypted, bool createFolder)
{
    (*getCategoryList(encrypted))[categoryName]=new Category();
    if(createFolder)
        createCategoryFolder(categoryName,encrypted);
    return true;
}

bool NotesInternals::createEntryFile(QString categoryName, Category::iterator entryIterator, bool encrypted)
{
    //content to be written to file
    QByteArray content;
    if(encrypted)
    {
        //for encrypted entries, content is comprised of hex representations of the initialization vector, as well as encrypted name and text of the entry
        //(one line each)
        QCA::InitializationVector iv(32);
        content=iv.toByteArray().toHex()+QByteArray("\n")
                +cryptoBuffer.encrypt(QCA::SecureArray((*entryIterator).first.toUtf8()),iv).toByteArray().toHex()+QByteArray("\n")
                +cryptoBuffer.encrypt(QCA::SecureArray((*entryIterator).second->entryText_.toByteArray()),iv).toByteArray().toHex();
    }
    else
    {
        //for unencrypted entries, content consists of plaintext entry name and text (first line is entry name)
        content=(*entryIterator).first.toUtf8()+QByteArray("\n")
                +(*entryIterator).second->entryText_.toByteArray();
    }
    //determine folder name based on category
    QString folderName=getCategoryFolderName(categoryName,encrypted);
    //determine file name based on hash of content
    QString contentHash=QString(hash256(QCA::SecureArray(content)));
    QString fileName=contentHash+".entry";
    QFile file(folderName+fileName);
    int i=0;
    //safeguard in case of hash collision or other trouble
    while(file.exists())
    {
        fileName=contentHash+"-"+QString::number(i++)+".entry";
        file.setFileName(folderName+fileName);
    }
    //write content
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
    //update fileName_ field of entry
    (*entryIterator).second->fileName_=fileName;
    return true;
}

bool NotesInternals::createCategoryFolder(QString categoryName, bool encrypted)
{
    //content to be written to file
    QByteArray content;
    if(encrypted)
    {
        //for encrypted categories, content is comprised of hex representations of the initialization vector, as well as encrypted name of the category
        //(one line each)
        QCA::InitializationVector iv(32);
        content=iv.toByteArray().toHex()+QByteArray("\n")
                +cryptoBuffer.encrypt(QCA::SecureArray(categoryName.toUtf8()),iv).toByteArray().toHex();
    }
    else
    {
        //for unencrypted categories, content consists of plaintext category name
        content=categoryName.toUtf8();
    }
    //determine folder name based on hash of category name
    QString nameHash=QString(hash256(QCA::SecureArray(categoryName.toUtf8())));
    QString pre=(encrypted?QString("./enc/"):QString("./plain/"));
    QString folderName=pre+nameHash+QString("/");
    QDir dir(folderName);
    int i=0;
    while(dir.exists())
    {
        folderName=pre+nameHash+QString("-")+QString::number(i++)+QString("/");
        dir.setPath(folderName);
    }
    QDir::mkpath(folderName);
    QFile file(folderName+"category");
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
    return true;
}

bool NotesInternals::updateEntryFile(QString categoryName, Category::iterator entryIterator, bool encrypted)
{
    QFile file(getCategoryFolderName(categoryName,encrypted)+(*entryIterator).second->fileName_);
    QString tmpFileName=(*entryIterator).second->fileName_+QString(".tmp");
    if(!file.rename(tmpFileName))
        return false;
    createEntryFile(categoryName,entryIterator,encrypted);
    file.setFileName(tmpFileName);
    file.remove();
    return true;
}

QString NotesInternals::getCategoryFolderName(QString categoryName, bool encrypted)
{
    return categoryFolderNameList_[std::pair<QString,bool>(categoryName,encrypted)];
}

bool NotesInternals::categoryExists(QString categoryName, bool encrypted)
{
    if(encrypted)
    {
        if(!encryptionActive_)
            return false;
        return (encryptedCategoryList_.find(categoryName)!=encryptedCategoryList_.end());
    }
    return (unencryptedCategoryList_.find(categoryName)!=unencryptedCategoryList_.end());
}

bool NotesInternals::entryExists(QString categoryName, QString entryName, bool encrypted)
{
    if(!categoryExists(categoryName,encrypted))
        return false;
    Category *category=(*getCategoryList(encrypted))[categoryName];
    return (category->find(entryName)!=category->end());
}

bool NotesInternals::createEntryFile(QString categoryName, QString entryName, bool encrypted)
{
    return createEntryFile(categoryName,getCategory(categoryName,encrypted)->find(entryName),encrypted);
}

bool NotesInternals::loadEntryFile(QString fileName, bool encrypted)
{
    QFile file(fileName);
    if(!file.exists())
        return false;
    QCA::SecureArray categoryName;
    QCA::SecureArray entryName;
    QCA::SecureArray entryText;
    file.open(QFile::ReadOnly);
    if(encrypted)
    {
        if(!encryptionActive_)
            return false;

        QByteArray ivHex;
        QCA::InitializationVector iv(32);
        ivHex=file.readLine();
        iv=QCA::hexToArray(QString(ivHex));
        QByteArray encryptedCategoryNameHex;
        QCA::SecureArray encryptedCategoryName;
        encryptedCategoryNameHex=file.readLine();
        encryptedCategoryName=QCA::hexToArray(QString(encryptedCategoryNameHex));
        cryptoBuffer.decrypt(encryptedCategoryName,categoryName,iv);
        QCA::SecureArray encryptedEntryName;
        QByteArray encryptedEntryNameHex;
        encryptedEntryNameHex=file.readLine();
        encryptedEntryName=QCA::hexToArray(QString(encryptedEntryNameHex));
        cryptoBuffer.decrypt(encryptedEntryName,entryName,iv);
        QCA::SecureArray encryptedText;
        QByteArray encryptedTextHex;
        encryptedTextHex=file.readLine();
        encryptedText=QCA::hexToArray(QString(encryptedTextHex));
        cryptoBuffer.encrypt(encryptedText,entryText,iv);
    }
    else
    {
        categoryName=QCA::SecureArray(file.readLine());
        entryName=QCA::SecureArray(file.readLine());
        entryText=QCA::SecureArray(file.readAll());
    }
    if(categoryName.size()==0 || entryName.size()==0)
        return false;
    createEntry(categoryName.toByteArray(),entryName.toByteArray(),entryText,encrypted,false);
    return true;
}

bool NotesInternals::updateEntryFile(QString categoryName, QString entryName, bool encrypted)
{
    return updateEntryFile(categoryName,getCategory(categoryName,encrypted)->find(entryName),encrypted);

}

Category *NotesInternals::getCategory(QString categoryName, bool encrypted)
{
    Category *category;
    if(encrypted)
        category=(Category*)(encryptedCategoryList_[categoryName]);
    else
        category=(Category*)(unencryptedCategoryList_[categoryName]);
    return category;
}

Entry *NotesInternals::getEntry(QString categoryName, QString entryName, bool encrypted)
{
    Entry *entry;
    entry=(Entry*)((*(getCategory(categoryName,encrypted)))[entryName]);
    return entry;
}

CategoryList *NotesInternals::getCategoryList(bool encrypted)
{
    return (encrypted?(&encryptedCategoryList_):(&unencryptedCategoryList_));
}
*/


Category::Category()
{

}

Category::~Category()
{
    for(EntriesMap::iterator i=entriesMap_.begin();i!=entriesMap_.end();++i)
        delete NotesInternals::getEntry(i);
}
