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
#include <QAbstractListModel>
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
    const EntriesMap* entriesMap() const{return &entriesMap_;}
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
    const CategoriesMap::const_iterator addCategory(QString categoryName);
    void removeCategory(CategoriesMap::const_iterator &categoryIterator);
    const CategoriesMap::const_iterator renameCategory(CategoriesMap::const_iterator &categoryIterator,QString newCategoryName);
    const EntriesMap::const_iterator addEntry(CategoriesMap::const_iterator &categoryIterator,QString entryName);
    void removeEntry(CategoriesMap::const_iterator &categoryIterator,EntriesMap::const_iterator &entryIterator);
    const EntriesMap::const_iterator renameEntry(CategoriesMap::const_iterator &categoryIterator,EntriesMap::const_iterator &entryIterator,QString newEntryName);
    const EntriesMap::const_iterator moveEntry(CategoriesMap::const_iterator &oldCategoryIterator,EntriesMap::const_iterator &entryIterator,CategoriesMap::const_iterator &newCategoryIterator);
    const EntriesMap::const_iterator modifyEntryText(CategoriesMap::const_iterator &categoryIterator,EntriesMap::const_iterator &entryIterator,QString newEntryText);

    bool enableEncryption(const QCA::SecureArray & password);
    void disableEncryption();
    bool encryptionEnabled() {return encryptionEnabled_;}

    const CategoriesMap* categoriesMap(){return &categoriesMap_;}

    static QString getCategoryName(const CategoriesMap::const_iterator &categoryIterator)
        {return (*(CategoriesMap::const_iterator)categoryIterator).first.first;}
    static bool getCategoryEncrypted(const CategoriesMap::const_iterator &categoryIterator)
        {return (*(CategoriesMap::const_iterator)categoryIterator).second->encrypted_;}
    static QString getCategoryFolderName(const CategoriesMap::const_iterator &categoryIterator)
        {return (*(CategoriesMap::const_iterator)categoryIterator).second->folderName_;}
    static const Category* getCategory(const CategoriesMap::const_iterator &categoryIterator)
        {return (*(CategoriesMap::const_iterator)categoryIterator).second;}
    static QString getEntryName(const EntriesMap::const_iterator &entryIterator)
        {return (*(EntriesMap::const_iterator)entryIterator).first.first;}
    static QString getEntryText(const EntriesMap::const_iterator &entryIterator)
        {return (*(EntriesMap::const_iterator)entryIterator).second->entryText_;}
    static QString getEntryFileName(const EntriesMap::const_iterator &entryIterator)
        {return (*(EntriesMap::const_iterator)entryIterator).second->fileName_;}
    static const Entry* getEntry(const EntriesMap::const_iterator &entryIterator)
        {return (*(EntriesMap::const_iterator)entryIterator).second;}


private:
    CryptoBuffer cryptoBuffer_;
    QCA::Hash hashFunction_;
    CategoriesMap categoriesMap_;
    bool encryptionEnabled_;

    void loadUnencryptedCategories();
    void loadEncryptedCategories();
    void removeEncryptedCategories();

    bool updateEntryFile(CategoriesMap::const_iterator &categoryIterator,EntriesMap::const_iterator &entryIterator);
    bool updateCategoryFile(CategoriesMap::const_iterator &categoryIterator);
    static Category* getCategory_(const CategoriesMap::const_iterator &categoryIterator)
        {return (*(CategoriesMap::const_iterator)categoryIterator).second;}
    static Entry* getEntry_(const EntriesMap::const_iterator &entryIterator)
        {return (*(EntriesMap::const_iterator)entryIterator).second;}
};

class CategoryListModel: public QAbstractListModel
{
    Q_OBJECT
    friend class NotesInternals;
public:
    CategoryListModel(QObject *parent):QAbstractListModel(parent){}
private:

};
#endif // NOTESINTERNALS_H
