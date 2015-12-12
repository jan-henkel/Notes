#ifndef NOTESINTERNALS_H
#define NOTESINTERNALS_H

#include <QObject>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <QStringList>
#include <QFile>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <Qca-qt5/QtCrypto/QtCrypto>
#include "cryptobuffer.h"

class Entry
{
    friend class NotesInternals;
private:
    QCA::SecureArray entryText_;
    QString fileName_;
};

typedef std::map<QString,Entry*> Category;
typedef std::map<QString,Category*> CategoryList;

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    enum Result {SUCCESS,CATEGORY_EXISTS,ENTRY_EXISTS,CATEGORY_DOES_NOT_EXIST,ENTRY_DOES_NOT_EXIST,ENCRYPTION_INACTIVE,WRONG_PASSWORD,PASSWORD_NOT_SET,FILE_ERROR};
    NotesInternals(QObject *parent=0);
    Result createCategory(QString categoryName,bool encrypted);
    Result createEntry(QString categoryName, QString entryName, QCA::SecureArray entryText, bool encrypted);
    Result deleteCategory(QString categoryName,bool encrypted);
    Result deleteEntry(QString categoryName,QString entryName, bool encrypted);
    Result renameCategory(QString oldCategoryName,QString newCategoryName, bool encrypted);
    Result renameEntry(QString categoryName,QString oldEntryName,QString newEntryName, bool encrypted);
    Result changeEntryCategory(QString entryName,QString oldCategoryName,QString newCategoryName, bool encrypted);
    Result getCategoryList(bool encrypted, QStringList &rList);
    Result getCategoryEntries(QString categoryName, bool encrypted, QStringList &rList);
    Result getEntryText(QString categoryName, QString entryName, bool encrypted, QCA::SecureArray &rText);
    Result toggleCategoryEncryption(QString categoryName, bool encrypted);
    Result loadPlainEntryFiles();
    Result loadEncryptedEntryFiles(const QCA::SecureArray &password);
    Result unloadEncryptedEntryFiles();
public slots:

private:
    QCA::Hash hashFunction;
    CategoryList unencryptedCategoryList_;
    CategoryList encryptedCategoryList_;
    std::map<std::pair<QString,bool>,QString> categoryFolderNameList_;
    CryptoBuffer cryptoBuffer;
    bool encryptionActive_;

    //auxiliary functions
    void hash256(const QCA::SecureArray &data, QByteArray &rHash);
    QByteArray hash256(const QCA::SecureArray &data);
    //void hash512(const QCA::SecureArray &data,QCA::SecureArray &rHash);

    //input requirements for bug free usage:

    //entry must not yet exist, encryption must be active if encrypted==true
    bool createEntry(CategoryList::iterator categoryIterator, QString entryName, const QCA::SecureArray &entryText, bool encrypted, bool saveFile);
    bool createEntry(QString categoryName, QString entryName, const QCA::SecureArray &entryText, bool encrypted, bool saveFile);
    //entry has to exist
    bool deleteEntry(CategoryList::iterator categoryIterator,Category::iterator entryIterator,bool encrypted, bool removeFile);
    bool deleteEntry(QString categoryName,QString entryName, bool encrypted, bool removeFile);

    bool createCategory(QString categoryName, bool encrypted, bool createFolder);
    bool createEntryFile(QString categoryName,Category::iterator entryIterator, bool encrypted);
    bool createCategoryFolder(QString categoryName,bool encrypted);
    bool updateEntryFile(QString categoryName,Category::iterator entryIterator, bool encrypted);
    bool updateEntryFile(QString categoryName,QString entryName, bool encrypted);

    bool categoryExists(QString categoryName,bool encrypted);
    bool entryExists(QString categoryName,QString entryName, bool encrypted);
    bool createEntryFile(QString categoryName,QString entryName, bool encrypted);
    bool loadEntryFile(QString fileName,bool encrypted);

    QString getCategoryFolderName(QString categoryName,bool encrypted);
    Category* getCategory(QString categoryName,bool encrypted);
    Entry* getEntry(QString categoryName,QString entryName, bool encrypted);
    CategoryList* getCategoryList(bool encrypted);
};

/*
class NotesInternals;
class Entry
{
    friend class NotesInternals;
public:
private:
    Entry();

    QString fileName_;
};

class UnencryptedEntry : public Entry
{
    friend class NotesInternals;
public:
    UnencryptedEntry(QString entryText,QString fileName) {entryText_=entryText;fileName_=fileName;}
    QString entryText(){return entryText_;}
private:
    QString entryText_;
};

class EncryptedEntry : public Entry
{
    friend class NotesInternals;
public:
    EncryptedEntry();

};

typedef std::map<QString,Entry*> Category;
typedef std::map<QString,Category*> CategoryList;

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    NotesInternals(QObject *parent);
public slots:
    void newEntry(QString categoryName,QString entryName,QString entryText,bool encrypted)
    {
        QString fileName=saveEntryFile(categoryName,entryName,entryText,encrypted);
        createEntry(categoryName,entryName,entryText,fileName,encrypted);
    }

    void delEntry(QString categoryName,QString entryName)
    {
        deleteEntry(categoryName,entryName,true);
    }

    void moveEntry(QString oldCategoryName,QString oldEntryName,QString newCategoryName,QString newEntryName)
    {
        modifyEntryNameAndCategory(oldCategoryName,oldEntryName,newCategoryName,newEntryName);
    }

    void moveCategory(QString oldCategoryName,QString newCategoryName);
private:
    CategoryList categoryList;
    QCryptographicHash hashFunction;
    QString calcHash(QString content);
    QString saveEntryFile(QString categoryName,QString entryName,QString entryText, bool encrypted);
    bool loadEntryFile(QString fileName,bool encrypted);
    bool categoryExists(QString categoryName) {return (categoryList.find(categoryName)!=categoryList.end());}
    void createCategory(QString categoryName) {Category* newCategory=new Category(); categoryList[categoryName]=newCategory;}
    bool renameCategory(QString categoryOldName,QString categoryNewName);
    bool entryExists(QString categoryName,QString entryName);
    bool createEntry(QString categoryName,QString entryName,QString entryText=QString(""),QString fileName=QString(""), bool encrypted=false);
    bool modifyEntryNameAndCategory(QString oldCategoryName,QString oldEntryName,QString newCategoryName,QString newEntryName);
    bool modifyEntryText(QString categoryName,QString entryName,QString newEntryText);
    bool deleteEntry(QString categoryName,QString entryName, bool deleteFile);
    bool deleteCategory(QString categoryName,bool deleteFiles);
};*/


#endif // NOTESINTERNALS_H
