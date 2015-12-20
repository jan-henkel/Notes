#ifndef CRYPTOBUFFER_H
#define CRYPTOBUFFER_H

#define DEFAULT_PBKDF2_ITERATIONS 10000

#include <QObject>
#include <Qca-qt5/QtCrypto/QtCrypto>
#include <QFile>

class CryptoInterface : public QObject
{
    Q_OBJECT
public:
    //enum listing various error codes
    enum Result {SUCCESS,WRONG_PASSWORD,NO_FILE,WRONG_FILE_FORMAT,KEY_NOT_SET};

    explicit CryptoInterface(QObject *parent = 0);

    //<--
    //read and decrypt master key and default initialization vector from file, using password
    Result readKeyIV(const QCA::SecureArray &password, QString fileName);
    //encrypt master key and default initialization vector with a key generated from password
    //write encrypted master key and default initialization vector to file
    Result writeKeyIV(const QCA::SecureArray &password, QString fileName);

    //in both cases return SUCCESS or error code
    //random initialization vectors and salt are used to encrypt / decrypt the master key and default IV
    //these are also saved to or read from the file (unencrypted) respectively
    //-->


    //set master key and default initialization vector
    void setKeyIV(const QCA::SymmetricKey &key,const QCA::SymmetricKey &iv);

    //set master key and default iv using QCAs pseudorandom number generator
    void setRandomKeyIV();

    //encrypt and decrypt using master key and default iv, return SUCCESS or error code
    //both ciphertext and plaintext variable are passed as source or target respectively
    Result encrypt(QCA::SecureArray &rCipher, const QCA::SecureArray &plain);
    Result decrypt(const QCA::SecureArray &cipher,QCA::SecureArray &rPlain);

    //encrypt and decrypt using master key and custom initialization vector, return SUCCESS or error code
    //both ciphertext and plaintext variable are passed as source or target respectively
    Result encrypt(QCA::SecureArray &rCipher, const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    Result decrypt(const QCA::SecureArray &cipher,QCA::SecureArray &rPlain,const QCA::InitializationVector &iv);

    //encrypt and decrypt using master key and default iv, return encrypted / decrypted array
    QCA::SecureArray encrypt(const QCA::SecureArray &plain);
    QCA::SecureArray decrypt(const QCA::SecureArray &cipher);

    //encrypt and decrypt using master key and custom iv, return encrypted / decrypted array
    QCA::SecureArray encrypt(const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    QCA::SecureArray decrypt(const QCA::SecureArray &cipher,const QCA::InitializationVector &iv);

    //remove master key and default iv
    void unsetKeyIV();

    //return whether master key and default iv are set
    bool keyIVSet() {return keyIVSet_;}

    //sanitize memory to prevent future readout of confidential data
    void scrambleMemory(QCA::SecureArray &memory,quint64 size);
signals:

public slots:
private:
    const int pbkdf2iterations_;

    //QCA initializer object
    QCA::Initializer initializer_;

    //master key
    QCA::SymmetricKey masterkey_;
    //default iv, may not be used at all, if user specifies an iv for each case (preferred).
    QCA::InitializationVector defaultiv_;

    //bool inidicating whether master key and default iv are set
    bool keyIVSet_;

    //encryption / decryption subroutines without failsafes
    //inputs are key and initialization vector to use as well as ciphertext and plaintext variables as source or target respectively
    void decryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, const QCA::SecureArray &ciphertext, QCA::SecureArray &rPlaintext);
    void encryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, QCA::SecureArray &rCiphertext, const QCA::SecureArray &plaintext);

    //auxiliary functions
    //decrypt master key and save it to rKey. for that, do the following: password and salt yield intermediate "long" key K, calculated with calcK.
    //in verifyK2andTruncateK the known array k2 is compared to second half of K for password verification, upon success the 2nd half of K is cut off.
    //the remaining half of K is used to decrypt the master key, using known initialization vector iv.
    bool verifyPasswordAndDecryptMasterKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &k2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rKey);
    void calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK);
    bool verifyK2AndTruncateK(const QCA::SecureArray &k2,QCA::SecureArray &k);

};

#endif // CRYPTOBUFFER_H
