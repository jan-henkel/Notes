#include "notesinternals.h"

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


/*
QString NotesInternals::calcHash(QString content)
{
    hashFunction.addData(content.toUtf8());
    return QString(hashFunction.result().toHex());
}

QString NotesInternals::saveEntryFile(QString categoryName, QString entryName, QString entryText, bool encrypted)
{
    QString encString=(encrypted?"[Encrypted Entry]\n":"[Unencrypted Entry]\n");
    QString entryContent=encString+categoryName+'\n'+entryName+'\n'+entryText;
    QString hash=calcHash(entryContent);
    QString fileName=hash;
    int i=0;
    QFile file(hash);
    while(file.exists())
    {
        fileName=hash+"-"+QString::number(i++);
        file.setFileName(fileName);
    }
    file.open(QFile::ReadWrite);
    file.write(entryContent.toUtf8());
    file.close();
    return fileName;
}

bool NotesInternals::loadEntryFile(QString fileName, bool encrypted)
{
    QFile file(fileName);
    if(!file.exists())
        return false;
    file.open(QFile::ReadOnly);
    QString content=QString::fromUtf8(file.readAll());
    file.close();
    QString categoryName,entryName,entryText;
    int j,k;
    j=content.indexOf('\n');
    if(j>=content.length() || j==-1)
        return false;
    k=content.indexOf('\n',j+1);
    if(k>=content.length() || k==-1)
        return false;
    categoryName=content.mid(0,j);
    entryText=content.mid(j+1,k);
    entryName=content.mid(k+1);
    createEntry(categoryName,entryName,entryText,fileName,encrypted);
}

bool NotesInternals::renameCategory(QString categoryOldName, QString categoryNewName) {
    if(!categoryExists(categoryOldName)) {return false;}
    if(categoryExists(categoryNewName)) {return false;}
    categoryList[categoryNewName]=categoryList[categoryOldName];
    categoryList.erase(categoryOldName);
    return true;
}

bool NotesInternals::entryExists(QString categoryName, QString entryName)
{
    if(!categoryExists(categoryName))
        return false;
    Category* category=categoryList[categoryName];
    return ((*category).find(entryName)!=(*category).end());
}

bool NotesInternals::createEntry(QString categoryName, QString entryName, QString entryText, QString fileName, bool encrypted)
{
    if(entryExists(categoryName,entryName))
        return false;
    if(!categoryExists(categoryName))
        createCategory(categoryName);
    (*(categoryList[categoryName]))[entryName]=new Entry(entryText,fileName,encrypted);
    return true;
}

bool NotesInternals::modifyEntryNameAndCategory(QString oldCategoryName, QString oldEntryName, QString newCategoryName, QString newEntryName)
{
    if(!entryExists(oldCategoryName,oldEntryName))
        return false;
    if(entryExists(newCategoryName,newEntryName))
        return false;
    Category *newCategory=categoryList[newCategoryName],*oldCategory=categoryList[oldCategoryName];
    (*newCategory)[newEntryName]=(*oldCategory)[oldEntryName];
    oldCategory->erase(oldEntryName);
    return true;
}

bool NotesInternals::modifyEntryText(QString categoryName, QString entryName, QString newEntryText)
{
    if(!entryExists(categoryName,entryName))
        return false;
    Category *category=categoryList[categoryName];
    Entry *entry=(*category)[entryName];
    entry->entryText_=newEntryText;
    QString newFileName=saveEntryFile(categoryName,entryName,newEntryText,entry->encrypted_);
    entry->fileName_=newFileName;
    return true;
}

bool NotesInternals::deleteEntry(QString categoryName, QString entryName, bool deleteFile)
{
    if(!entryExists(categoryName,entryName))
        return false;
    Category *category=categoryList[categoryName];
    Entry *entry=(*category)[entryName];
    category->erase(entryName);
    if(deleteFile)
    {
        QFile file(entry->fileName_);
        if(file.exists())
            file.remove();
    }
    delete entry;
    return true;
}

bool NotesInternals::deleteCategory(QString categoryName, bool deleteFiles)
{
    Category *category=categoryList[categoryName];
    for(Category::iterator i=category->begin();i!=category->end();++i)
    {
        deleteEntry(categoryName,(*i).first,deleteFiles);
    }
    delete category;
}*/

/*

QString NotesInternals::saveEntryFile(QString title, QString tags, QString text)
{
    QString outputText=title+"\n"+tags+"\n"+text;
    hashFunction.addData(outputText.toUtf8());
    QString fileName(hashFunction.result().toHex());
    QFile file("./entries/"+fileName);
    if(file.exists())
    {
        QString newFileName;
        int i=0;
        while(file.exists())
        {
            newFileName=fileName+"-"+QString::number(i++);
            file.setFileName("./entries/"+newFileName);
        }
        fileName=newFileName;
    }
    file.open(QFile::ReadWrite);
    file.write(outputText.toUtf8());
    file.close();
    return fileName;
}


void NotesInternals::readEntryFiles()
{
    QString filename;
    QDirIterator i("./entries",QDir::Files);
    while(i.hasNext())
    {
        filename=i.next();
        readEntryFile(filename);
    }
}
*/
