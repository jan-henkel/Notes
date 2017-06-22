#ifndef NOTESINTERNALS_H
#define NOTESINTERNALS_H

#include <QObject>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <QStringList>
#include <QFile>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QAbstractListModel>
#include <QMetaType>
#include <QRegularExpression>
#include "cryptointerface.h"

class EntryContent;
class CategoryContent;

//categories and entries are sorted by name and creation date
template <typename Content>
class Datum
{
    friend class NotesInternals;
public:
    QString name;
    QDateTime date;
    Datum(QString name,QDateTime date,std::shared_ptr<Content> content):name(name),date(date),content(content) {}
    bool operator<(const Datum<Content> other) const {
        if(QString::localeAwareCompare(name.toLower(),other.name.toLower())<0)
            return true;
        else
        {
            if(name.toLower()==other.name.toLower())
            {
                if(QString::localeAwareCompare(name,other.name)<0)
                    return true;
                else
                {
                    if(name==other.name)
                    {
                        if(date<other.date)
                            return true;
                        else if(date==other.date)
                            return (content<other.content);
                    }
                }
            }
        }
        return false;
    }
    bool operator==(const Datum<Content> other) const {
        return (name==other.name && date==other.date && content==other.content);
    }
    bool operator!=(const Datum<Content> other) const {
        return (name!=other.name || date!=other.date || content!=other.content);
    }
private:
    std::shared_ptr<Content> content;
};

//Entry and Category types
typedef Datum<EntryContent> Entry;
typedef Datum<CategoryContent> Category;

//set types to store them
typedef std::set<Entry> EntrySet;
typedef std::set<Category> CategorySet;

//Entry class
class EntryContent
{
    friend class NotesInternals;
    friend class CategoryContent;
private:
    CryptoPP::SecByteBlock text_; //unencrypted text of the entry
    QString fileName_;  //file name without path associated with the entry
};

//Category class contains information about whether the category is
class CategoryContent
{
    friend class NotesInternals;
public:
    CategoryContent();
    ~CategoryContent();
    const EntrySet* entrySet() const{return &entrySet_;} //return constant entry set, so other classes can iterate over entries
private:
    bool encrypted_;        //category encrypted
    QString path_;    //folder path associated with the category, relative to application path
    EntrySet entrySet_;     //set of entries belonging to category
};

class NotesInternals : public QObject
{
    Q_OBJECT
public:
    NotesInternals(QObject *parent);

    //functions to manipulate categories and entries. selected categories / entries and associated files get updated accordingly

    //add new category with name categoryName
    const Category addCategory(QString categoryName);
    //remove category. returns false if passed category is invalid
    bool removeCategory(Category &category);
    //rename category to newCategoryName. returns updated category if successful, invalid otherwise
    const Category renameCategory(Category &category, QString newCategoryName);
    //encrypt or decrypt category
    const Category toggleCategoryEncryption(Category category);
    //add entry of name entryName to category. returns newly generated entry if successful, invalid otherwise
    //(e.g. if passed category is invalid)
    const Entry addEntry(Category &category, QString entryName);
    //remove entry corresponding to entry from category specified by category. returns true on success,
    //false if either of the arguments passed is invalid
    bool removeEntry(Category &category, Entry &entry);
    //rename entry corresponding to (category,entry) to newEntryName. return updated entry,
    //invalid if any of the arguments passed is invalid
    const Entry renameEntry(Category &category, Entry &entry, QString newEntryName);
    //move entry specified by (oldCategory,entry) to category.
    //return updated entry if successful, invalid otherwise
    const Entry moveEntry(Category &oldCategory, Entry &entry, Category &newCategory);
    //change entry text of entry specified by (category,entry) to newEntryText.
    //return entry if successful, invalid otherwise
    const Entry modifyEntryText(Category category, Entry entry, CryptoPP::SecByteBlock newEntryText);

    //enable encryption, add encrypted categories. returns false if password is wrong or not set
    bool enableEncryption(const CryptoPP::SecByteBlock & password);
    //disable encryption, remove encrypted categories
    void disableEncryption();

    //create new password and master key
    bool createNewMasterKey(const CryptoPP::SecByteBlock & newPassword);
    bool createNewPassword(const CryptoPP::SecByteBlock &newPassword);
    bool masterKeyExists()
        {return QFile("./enc/masterkey").exists();}

    bool encryptionEnabled() {return encryptionEnabled_;}

    //return constant category set so other classes can iterate over categories
    const CategorySet* categorySet(){return &categorySet_;}

    //auxiliary functions
    template <class Content>
    static std::shared_ptr<const Content> getContent(const Datum<Content> &datum) {
        return datum.content;
    }
    static bool isEncrypted(const Category &category)
        {return category.content?category.content->encrypted_:false;}
    static QString getPath(const Category &category)
        {return category.content?category.content->path_:QString("");}
    static CryptoPP::SecByteBlock getText(const Entry &entry)
        {return entry.content?entry.content->text_:CryptoPP::SecByteBlock(0,0);}
    static QString getFileName(const Entry &entry)
        {return entry.content?entry.content->fileName_:QString("");}

    //return invalid category and entry prototypes
    static Category invalidCategory() {return Category(QString(""),QDateTime::fromMSecsSinceEpoch(0),0);}
    static Entry invalidEntry() {return Entry(QString(""),QDateTime::fromMSecsSinceEpoch(0),0);}

    //functions to check whether items are valid (i.e. are present in their respective sets)
    bool isValid(const Category &category)
        {return (categorySet_.find(category)!=categorySet_.end());}
    bool isValid(const Category &category,const Entry &entry)
        {return (isValid(category) && (category.content->entrySet_.find(entry)!=category.content->entrySet_.end()));}

    //functions to select new categories / entries to track (important for GUIs)
    //selected objects are updated according to any changes made, e.g. set to invalid if the corresponding object is deleted
    void selectCategory(const Category& category);
    void selectEntry(const Entry& entry);

    //functions to return the currently selected (tracked) items
    Category currentCategory(){return currentCategory_;}
    Entry currentEntry(){return currentEntry_;}

    //functions to manipulate currently selected items
    bool removeCurrentCategory()
        {return removeCategory(currentCategory_);}
    const Category renameCurrentCategory(QString newCategoryName)
        {return renameCategory(currentCategory_,newCategoryName);}
    const Category toggleCurrentCategoryEncryption()
        {return toggleCategoryEncryption(currentCategory_);}
    const Entry addEntryToCurrentCategory(QString entryName)
        {return addEntry(currentCategory_,entryName);}
    bool removeCurrentEntry()
        {return removeEntry(currentCategory_,currentEntry_);}
    const Entry renameCurrentEntry(QString newEntryName)
        {return renameEntry(currentCategory_,currentEntry_,newEntryName);}
    const Entry moveCurrentEntry(Category &newCategory)
        {return moveEntry(currentCategory_,currentEntry_,newCategory);}
    const Entry modifyCurrentEntryText(CryptoPP::SecByteBlock newEntryText)
        {return modifyEntryText(currentCategory_,currentEntry_,newEntryText);}

signals:
    //signals to keep track of changes in category set, selected items
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
    CryptoPP::SHA256 hashFunction_;

    //set of categories
    CategorySet categorySet_;

    //currently selected (tracked) items
    Category currentCategory_;
    Entry currentEntry_;

    //enryption currently enabled
    bool encryptionEnabled_;

    //load all encrypted or all unencrypted categories and entries from files
    void loadCategories(bool encrypted);
    void loadUnencryptedCategories();
    void loadEncryptedCategories();
    //remove all encrypted categories
    void removeEncryptedCategories();

    //function to update entry file. creates file if it doesn't exist yet (useful for creating new entries)
    bool updateEntryFile(Category category, Entry entry);
    //function to update category file and folder. creates file and folder if it doesn't exist yet (useful for new categories)
    bool updateCategoryFile(Category category);

    //extract pointers to non-constant category or entry content
};


#endif // NOTESINTERNALS_H
