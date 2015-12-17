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
#include <Qca-qt5/QtCrypto/QtCrypto>
#include <QAbstractListModel>
#include "cryptobuffer.h"

class Entry;
class Category;

typedef std::pair<QString,QDateTime> NameDate;
typedef std::pair<NameDate,Entry*> EntryPair;
typedef std::pair<NameDate,Category*> CategoryPair;
typedef std::set<EntryPair> EntriesMap;
typedef std::set<CategoryPair> CategoriesMap;


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

class CategoryListModel: public QAbstractListModel
{
    Q_OBJECT
    friend class NotesInternals;
public:
    CategoryListModel(QObject *parent):QAbstractListModel(parent){}
private:
    void categoriesFromMap(CategoriesMap* map,CategoryPair &keep);
    void categoriesFromMap(CategoriesMap* map);
    std::vector<CategoryPair> categoryPairs_;
};

class EntryListModel: public QAbstractListModel
{
    Q_OBJECT
    friend class NotesInternals;
public:
    EntryListModel(QObject *parent):QAbstractListModel(parent){}
private:
    void entriesFromMap(EntriesMap* map,QString filter,CategoryPair &keep);
    void entriesFromMap(EntriesMap* map,QString filter);
    std::vector<EntryPair> entryPairs_;
};

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    NotesInternals(QObject *parent);
    const CategoryPair addCategory(QString categoryName);
    bool removeCategory(CategoryPair &categoryPair);
    const CategoryPair renameCategory(CategoryPair &categoryPair,QString newCategoryName);
    const EntryPair addEntry(CategoryPair &categoryPair,QString entryName);
    bool removeEntry(CategoryPair &categoryPair,EntryPair &entryPair);
    const EntryPair renameEntry(CategoryPair &categoryPair,EntryPair &entryPair,QString newEntryName);
    const EntryPair moveEntry(CategoryPair &oldCategoryPair,EntryPair &entryPair,CategoryPair &newCategoryPair);
    const EntryPair modifyEntryText(CategoryPair &categoryPair,EntryPair &entryPair,QString newEntryText);

    bool enableEncryption(const QCA::SecureArray & password);
    void disableEncryption();
    bool encryptionEnabled() {return encryptionEnabled_;}

    const CategoriesMap* categoriesMap(){return &categoriesMap_;}

    static QString getCategoryName(const CategoryPair &categoryPair)
        {return categoryPair.first.first;}
    static QString getCategoryDate(const CategoryPair &categoryPair)
        {return categoryPair.first.second.toString();}
    static bool getCategoryEncrypted(const CategoryPair &categoryPair)
        {return categoryPair.second->encrypted_;}
    static QString getCategoryFolderName(const CategoryPair &categoryPair)
        {return categoryPair.second->folderName_;}
    static const Category* getCategory(const CategoryPair &categoryPair)
        {return categoryPair.second;}
    static QString getEntryName(const EntryPair &entryPair)
        {return entryPair.first.first;}
    static QString getEntryDate(const EntryPair &entryPair)
        {return entryPair.first.second.toString();}
    static QString getEntryText(const EntryPair &entryPair)
        {return entryPair.second->entryText_;}
    static QString getEntryFileName(const EntryPair &entryPair)
        {return entryPair.second->fileName_;}
    static const Entry* getEntry(const EntryPair &entryPair)
        {return entryPair.second;}

    static CategoryPair invalidCategoryPair() {return CategoryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);}
    static EntryPair invalidEntryPair() {return EntryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);}

    bool isValid(const CategoryPair &categoryPair)
        {return (categoriesMap_.find(categoryPair)!=categoriesMap_.end());}
    bool isValid(const CategoryPair &categoryPair,const EntryPair &entryPair)
        {return (isValid(categoryPair) && (getCategory(categoryPair)->entriesMap_.find(entryPair)!=getCategory(categoryPair)->entriesMap_.end()));}
private:
    CryptoBuffer cryptoBuffer_;
    QCA::Hash hashFunction_;
    CategoriesMap categoriesMap_;
    //CategoryListModel categoryModel_;
    //EntryListModel entryModel_;

    bool encryptionEnabled_;

    void loadCategories(bool encrypted);
    void loadUnencryptedCategories();
    void loadEncryptedCategories();
    void removeEncryptedCategories();

    bool updateEntryFile(CategoryPair &categoryPair,EntryPair &entryPair);
    bool updateCategoryFile(CategoryPair &categoryPair);
    static Category* getCategory_(const CategoryPair &categoryPair)
        {return categoryPair.second;}
    static Entry* getEntry_(const EntryPair &entryPair)
        {return entryPair.second;}
};


#endif // NOTESINTERNALS_H
