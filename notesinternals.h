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
#include <QDateTime>
#include <QDirIterator>
#include <Qca-qt5/QtCrypto/QtCrypto>
#include "cryptobuffer.h"

class Entry;
class Category;

typedef std::pair<QString,QDateTime> NameDate;
typedef std::multimap<NameDate,Entry*> EntriesMap;
typedef std::multimap<NameDate,Category*> CategoriesMap;


class Entry
{
    friend class NotesInternals;
    friend class Category;
private:
    QString entryText_;
    QString fileName_;
};

class Category
{
    friend class NotesInternals;
public:
    Category();
    ~Category();
    const EntriesMap* entriesMap(){return &entriesMap_;}
private:
    bool encrypted_;
    QString folderName_;
    EntriesMap entriesMap_;
};

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    NotesInternals(QObject *parent);
    const CategoriesMap::iterator addCategory(QString categoryName);
    void removeCategory(CategoriesMap::iterator &categoryIterator);
    const CategoriesMap::iterator renameCategory(CategoriesMap::iterator &categoryIterator,QString newCategoryName);
    const EntriesMap::iterator addEntry(CategoriesMap::iterator &categoryIterator,QString entryName);
    void removeEntry(CategoriesMap::iterator &categoryIterator,EntriesMap::iterator &entryIterator);
    const EntriesMap::iterator renameEntry(CategoriesMap::iterator &categoryIterator,EntriesMap::iterator &entryIterator,QString newEntryName);
    const EntriesMap::iterator moveEntry(CategoriesMap::iterator &oldCategoryIterator,EntriesMap::iterator &entryIterator,CategoriesMap::iterator &newCategoryIterator);
    const EntriesMap::iterator modifyEntryText(CategoriesMap::iterator &categoryIterator,EntriesMap::iterator &entryIterator,QString newEntryText);

    bool enableEncryption(const QCA::SecureArray & password);
    void disableEncryption();
    bool encryptionEnabled() {return encryptionEnabled_;}

    const CategoriesMap* categoriesMap(){return &categoriesMap_;}

    static QString getCategoryName(const CategoriesMap::iterator &categoryIterator)
        {return (*(CategoriesMap::iterator)categoryIterator).first.first;}
    static bool getCategoryEncrypted(const CategoriesMap::iterator &categoryIterator)
        {return (*(CategoriesMap::iterator)categoryIterator).second->encrypted_;}
    static QString getCategoryFolderName(const CategoriesMap::iterator &categoryIterator)
        {return (*(CategoriesMap::iterator)categoryIterator).second->folderName_;}
    static const Category* getCategory(const CategoriesMap::iterator &categoryIterator)
        {return (*(CategoriesMap::iterator)categoryIterator).second;}
    static QString getEntryName(const EntriesMap::iterator &entryIterator)
        {return (*(EntriesMap::iterator)entryIterator).first.first;}
    static QString getEntryText(const EntriesMap::iterator &entryIterator)
        {return (*(EntriesMap::iterator)entryIterator).second->entryText_;}
    static QString getEntryFileName(const EntriesMap::iterator &entryIterator)
        {return (*(EntriesMap::iterator)entryIterator).second->fileName_;}
    static const Entry* getEntry(const EntriesMap::iterator &entryIterator)
        {return (*(EntriesMap::iterator)entryIterator).second;}
private:
    CryptoBuffer cryptoBuffer_;
    QCA::Hash hashFunction_;
    CategoriesMap categoriesMap_;
    bool encryptionEnabled_;

    void loadUnencryptedCategories();
    void loadEncryptedCategories();
    void removeEncryptedCategories();

    bool updateEntryFile(CategoriesMap::iterator &categoryIterator,EntriesMap::iterator &entryIterator);
    bool updateCategoryFile(CategoriesMap::iterator &categoryIterator);
    static Category* getCategory_(const CategoriesMap::iterator &categoryIterator)
        {return (*(CategoriesMap::iterator)categoryIterator).second;}
    static Entry* getEntry_(const EntriesMap::iterator &entryIterator)
        {return (*(EntriesMap::iterator)entryIterator).second;}
};


#endif // NOTESINTERNALS_H
