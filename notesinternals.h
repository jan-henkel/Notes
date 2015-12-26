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
#include <QMetaType>
#include "cryptointerface.h"

class Entry;
class Category;

//categories and entries are sorted by name and creation date
typedef std::pair<QString,QDateTime> NameDate;
//pair types store both name and date, as well as a pointer to the content
typedef std::pair<NameDate,Entry*> EntryPair;
typedef std::pair<NameDate,Category*> CategoryPair;

struct compare_entry
{
    bool operator() (const EntryPair &l,const EntryPair &r) const
    {
        QString ll=l.first.first.toLower();
        QString lr=r.first.first.toLower();
        if(QString::localeAwareCompare(ll,lr)<0)
            return true;
        else
        {
            if(ll==lr)
            {
                if(QString::localeAwareCompare(l.first.first,r.first.first)<0)
                {
                    return true;
                }
                else
                {
                    if(l.first.first==r.first.first && l.first.second<r.first.second)
                        return true;
                }
            }
        }
        return false;
    }
};

struct compare_category
{
    bool operator() (const CategoryPair &l,const CategoryPair &r) const
    {
        QString ll=l.first.first.toLower();
        QString lr=r.first.first.toLower();
        if(QString::localeAwareCompare(ll,lr)<0)
            return true;
        else
        {
            if(ll==lr)
            {
                if(QString::localeAwareCompare(l.first.first,r.first.first)<0)
                {
                    return true;
                }
                else
                {
                    if(l.first.first==r.first.first && l.first.second<r.first.second)
                        return true;
                }
            }
        }
        return false;
    }
};

//set types store pairs
typedef std::set<EntryPair,compare_entry> EntrySet;
typedef std::set<CategoryPair,compare_category> CategorySet;

//Entry class
class Entry
{
    friend class NotesInternals;
    friend class Category;
private:
    QCA::SecureArray entryText_; //unencrypted text of the entry
    QString fileName_;  //file name without path associated with the entry
};

//Category class contains information about whether the category is
class Category
{
    friend class NotesInternals;
public:
    Category();
    ~Category();
    const EntrySet* entrySet() const{return &entrySet_;} //return constant entry set, so other classes can iterate over entries
private:
    bool encrypted_;        //category encrypted
    QString folderName_;    //folder path associated with the category, relative to application path
    EntrySet entrySet_;     //set of entry pairs belonging to category
};

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    NotesInternals(QObject *parent);

    //functions to manipulate categories and entries. selected pairs and associated files get updated accordingly

    //add new category with name categoryName
    const CategoryPair addCategory(QString categoryName);
    //remove category specidied by pair. returns false if passed category pair is invalid
    bool removeCategory(CategoryPair &categoryPair);
    //rename category specified by pair to newCategoryName. returns updated category pair if successful, invalid pair otherwise
    const CategoryPair renameCategory(CategoryPair &categoryPair,QString newCategoryName);
    //add entry of name entryName to category specified by pair. returns newly generated entry pair if successful, invalid pair otherwise
    //(e.g. if passed category pair is invalid)
    const EntryPair addEntry(CategoryPair &categoryPair,QString entryName);
    //remove entry corresponding to entryPair from category specified by categoryPair. returns true on success,
    //false if either of the pairs passed is invalid
    bool removeEntry(CategoryPair &categoryPair,EntryPair &entryPair);
    //rename entry corresponding to (categoryPair,entryPair) to newEntryName. return updated entry pair,
    //invalid pair if any of the pairs passed is invalid
    const EntryPair renameEntry(CategoryPair &categoryPair,EntryPair &entryPair,QString newEntryName);
    //move entry specified by (oldCategoryPair,entryPair) to category determined by newCategoryPair.
    //return updated entry pair if successful, invalid pair otherwise
    const EntryPair moveEntry(CategoryPair &oldCategoryPair,EntryPair &entryPair,CategoryPair &newCategoryPair);
    //change entry text of entry specified by (categoryPair,entryPair) to newEntryText.
    //return entry pair if successful, invalid pair otherwise
    const EntryPair modifyEntryText(CategoryPair categoryPair, EntryPair entryPair, QCA::SecureArray newEntryText);

    //enable encryption, add encrypted categories. returns false if password is wrong or not set
    bool enableEncryption(const QCA::SecureArray & password);
    //disable encryption, remove encrypted categories
    void disableEncryption();

    //create new password and master key
    bool createNewMasterKey(const QCA::SecureArray & newPassword);
    bool createNewPassword(const QCA::SecureArray & newPassword);
    bool masterKeyExists()
        {return QFile("./enc/masterkey").exists();}

    bool encryptionEnabled() {return encryptionEnabled_;}

    //return constant category set so other classes can iterate over categories
    const CategorySet* categorySet(){return &categorySet_;}

    //auxiliary functions to extract information from category pairs and entry pairs
    //for invalid pairs and string return type, return empty string
    //returned pointers are const
    static QString getCategoryName(const CategoryPair &categoryPair)
        {return categoryPair.first.first;}
    static QDateTime getCategoryDate(const CategoryPair &categoryPair)
        {return categoryPair.first.second;}
    static bool getCategoryEncrypted(const CategoryPair &categoryPair)
        {return categoryPair.second?categoryPair.second->encrypted_:false;}
    static QString getCategoryFolderName(const CategoryPair &categoryPair)
        {return categoryPair.second?categoryPair.second->folderName_:QString("");}
    static const Category* getCategory(const CategoryPair &categoryPair)
        {return categoryPair.second;}
    static QString getEntryName(const EntryPair &entryPair)
        {return entryPair.first.first;}
    static QDateTime getEntryDate(const EntryPair &entryPair)
        {return entryPair.first.second;}
    static QCA::SecureArray getEntryText(const EntryPair &entryPair)
        {return entryPair.second?entryPair.second->entryText_:QCA::SecureArray(0,0);}
    static QString getEntryFileName(const EntryPair &entryPair)
        {return entryPair.second?entryPair.second->fileName_:QString("");}
    static const Entry* getEntry(const EntryPair &entryPair)
        {return entryPair.second;}

    //return invalid category pair and entry pair prototypes
    static CategoryPair invalidCategoryPair() {return CategoryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);}
    static EntryPair invalidEntryPair() {return EntryPair(NameDate(QString(""),QDateTime::fromMSecsSinceEpoch(0)),0);}

    //functions to check whether pairs are valid
    //for categories: check if category pair is in categorySet_
    bool isValid(const CategoryPair &categoryPair)
        {return (categorySet_.find(categoryPair)!=categorySet_.end());}
    //for entries: check if category pair is valid, if so check whether entry pair is in entrySet_ of the category
    bool isValid(const CategoryPair &categoryPair,const EntryPair &entryPair)
        {return (isValid(categoryPair) && (getCategory(categoryPair)->entrySet_.find(entryPair)!=getCategory(categoryPair)->entrySet_.end()));}

    //functions to select new pairs to track (important for GUIs)
    //selected pairs are updated according to any changes made, e.g. set to invalid pairs if the corresponding pair is deleted
    void selectCategory(const CategoryPair& categoryPair);
    void selectEntry(const EntryPair& entryPair);

    //functions to return the currently selected (tracked) pairs
    CategoryPair currentCategoryPair(){return currentCategoryPair_;}
    EntryPair currentEntryPair(){return currentEntryPair_;}

    //functions to manipulate currently selected pairs
    bool removeCurrentCategory()
        {return removeCategory(currentCategoryPair_);}
    const CategoryPair renameCurrentCategory(QString newCategoryName)
        {return renameCategory(currentCategoryPair_,newCategoryName);}
    const EntryPair addEntryToCurrentCategory(QString entryName)
        {return addEntry(currentCategoryPair_,entryName);}
    bool removeCurrentEntry()
        {return removeEntry(currentCategoryPair_,currentEntryPair_);}
    const EntryPair renameCurrentEntry(QString newEntryName)
        {return renameEntry(currentCategoryPair_,currentEntryPair_,newEntryName);}
    const EntryPair moveCurrentEntry(CategoryPair &newCategoryPair)
        {return moveEntry(currentCategoryPair_,currentEntryPair_,newCategoryPair);}
    const EntryPair modifyCurrentEntryText(QCA::SecureArray newEntryText)
        {return modifyEntryText(currentCategoryPair_,currentEntryPair_,newEntryText);}
signals:
    //signals to keep track of changes in category set, selected pairs
    //meant to notify GUI
    void categoryListChanged();
    void categorySelectionChanged();
    void categoryContentChanged();
    void entrySelectionChanged();
    void entryContentChanged();
private:
    //crypto interface to take care of encrypted files
    CryptoInterface cryptoInterface_;
    //hash function used in naming files and folders
    QCA::Hash hashFunction_;

    //set of categories
    CategorySet categorySet_;

    //currently selected (tracked) pairs
    CategoryPair currentCategoryPair_;
    EntryPair currentEntryPair_;

    //enryption currently enabled
    bool encryptionEnabled_;

    //load all encrypted or all unencrypted categories and entries from files
    void loadCategories(bool encrypted);
    void loadUnencryptedCategories();
    void loadEncryptedCategories();
    //remove all encrypted categories
    void removeEncryptedCategories();

    //function to update entry file. creates file if it doesn't exist yet (useful for creating new entries)
    bool updateEntryFile(CategoryPair categoryPair,EntryPair entryPair);
    //function to update category file and folder. creates file and folder if it doesn't exist yet (useful for new categories)
    bool updateCategoryFile(CategoryPair categoryPair);

    //functions to extract pointers to non-constant category or entry associated with pair
    static Category* getCategory_(const CategoryPair &categoryPair)
        {return categoryPair.second;}
    static Entry* getEntry_(const EntryPair &entryPair)
        {return entryPair.second;}
};


#endif // NOTESINTERNALS_H
