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
    //just to help future compatibility with crypto++ and different encryption schemes
    enum EncryptionScheme {PBKDF2_SHA1_AES256=0};

    explicit CryptoInterface(QObject *parent = 0);

    //read and decrypt master key from file, using password
    Result readMasterKey(const QCA::SecureArray &password, QString fileName);
    //encrypt master key with a key generated from password
    //write encrypted master key to file
    Result saveMasterKey(const QCA::SecureArray &password, QString fileName);

    //in both cases return SUCCESS or error code
    //a password, random initialization vector and salt are used to encrypt / decrypt the master key

    //set master key
    void setMasterKey(const QCA::SymmetricKey &key);

    //set iterations
    void setIterations(qint32 iterations) {pbkdf2iterations_=iterations;}

    //set encryption scheme (effectively a dummy, since only 1 value is available)
    void setEncryptionScheme(EncryptionScheme scheme) {encryptionScheme_=scheme;}

    //set master key using QCAs pseudorandom number generator
    void setRandomMasterKey();

    //encrypt and decrypt using master key and initialization vector iv, return SUCCESS or error code
    //both ciphertext and plaintext variable are passed as source or target respectively
    Result encrypt(QCA::SecureArray &rCipher, const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    Result decrypt(const QCA::SecureArray &cipher,QCA::SecureArray &rPlain,const QCA::InitializationVector &iv);

    //encrypt and decrypt using master key and custom iv, return encrypted / decrypted array
    QCA::SecureArray encrypt(const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    QCA::SecureArray decrypt(const QCA::SecureArray &cipher,const QCA::InitializationVector &iv);

    //discard master key
    void unsetMasterKey();

    //return whether master key is set
    bool masterKeySet() {return masterKeySet_;}

    //sanitize memory to prevent future readout of confidential data
    void scrambleMemory(QCA::SecureArray &memory,quint64 size);
signals:

public slots:
private:
    qint32 pbkdf2iterations_;
    EncryptionScheme encryptionScheme_;

    //QCA initializer object
    QCA::Initializer initializer_;

    //master key
    QCA::SymmetricKey masterkey_;

    //bool inidicating whether master key is set
    bool masterKeySet_;

    //encryption / decryption subroutines without failsafes
    //inputs are key and initialization vector to use as well as ciphertext and plaintext variables as source or target respectively
    void decryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, const QCA::SecureArray &ciphertext, QCA::SecureArray &rPlaintext);
    void encryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, QCA::SecureArray &rCiphertext, const QCA::SecureArray &plaintext);

    //auxiliary functions
    //decrypt master key and save it to rKey. for that, do the following: password and salt yield intermediate "long" key K, calculated with calcK.
    //in verifyK2 the known array hk2 is compared to the sha1-hash of the second half of K for password verification
    //the remaining half of K is used to decrypt the master key, using known initialization vector iv.
    bool verifyPasswordAndDecryptMasterKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &hk2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rK);
    void calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK1, QCA::SecureArray &rK2);
    bool verifyK2(const QCA::SecureArray &hk2, QCA::SecureArray &k2);

};

#endif // CRYPTOBUFFER_H
