#ifndef CRYPTOBUFFER_H
#define CRYPTOBUFFER_H

#define DEFAULT_PBKDF2_ITERATIONS 100000

#include <QObject>
#include <cryptopp/cryptlib.h>
#include <cryptopp/secblock.h>
#include <cryptopp/algparam.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/misc.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/filters.h>
#include <QFile>
#include <QByteArray>
#include <algorithm>

using CryptoPP::byte;

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
    Result readMasterKey(const CryptoPP::SecByteBlock &password, QString fileName);
    //encrypt master key with a key generated from password
    //write encrypted master key to file
    Result saveMasterKey(const CryptoPP::SecByteBlock &password, QString fileName);

    //in both cases return SUCCESS or error code
    //a password, random initialization vector and salt are used to encrypt / decrypt the master key

    //set master key

    void setMasterKey(const CryptoPP::SecByteBlock &key);

    //set iterations
    void setIterations(qint32 iterations) {pbkdf2iterations_=iterations;}

    //set encryption scheme (effectively a dummy, since only 1 value is available)
    void setEncryptionScheme(EncryptionScheme scheme) {encryptionScheme_=scheme;}

    //set master key using a pseudorandom number generator
    void setRandomMasterKey();
    //encrypt and decrypt using master key and initialization vector iv, return SUCCESS or error code
    //both ciphertext and plaintext variable are passed as source or target respectively
    Result encrypt(CryptoPP::SecByteBlock &rCipher, const CryptoPP::SecByteBlock &plain,const CryptoPP::SecByteBlock &iv);
    Result decrypt(const CryptoPP::SecByteBlock &cipher,CryptoPP::SecByteBlock &rPlain,const CryptoPP::SecByteBlock &iv);

    //encrypt and decrypt using master key and custom iv, return encrypted / decrypted array
    CryptoPP::SecByteBlock encrypt(const CryptoPP::SecByteBlock &plain, const CryptoPP::SecByteBlock &iv);
    CryptoPP::SecByteBlock decrypt(const CryptoPP::SecByteBlock &cipher,const CryptoPP::SecByteBlock &iv);

    //discard master key
    void unsetMasterKey();

    //return whether master key is set
    bool masterKeySet() {return masterKeySet_;}

    static CryptoPP::SecByteBlock toSecBlock(const QByteArray& arr) {return CryptoPP::SecByteBlock((const byte*)arr.constData(),arr.size());}
    static QByteArray toTmpByteArray(const CryptoPP::SecByteBlock& block) {return QByteArray::fromRawData((const char*)block.BytePtr(),block.SizeInBytes());}
    static QByteArray toPermByteArray(const CryptoPP::SecByteBlock& block) {return QByteArray((const char*)block.BytePtr(),block.SizeInBytes());}

    static void randomize(CryptoPP::SecByteBlock& block) {CryptoPP::AutoSeededRandomPool rng; rng.GenerateBlock(block.BytePtr(),block.SizeInBytes());}
signals:

public slots:
private:
    qint32 pbkdf2iterations_;
    EncryptionScheme encryptionScheme_;

    //master key
    CryptoPP::SecByteBlock masterkey_;

    //bool inidicating whether master key is set
    bool masterKeySet_;

    //encryption / decryption subroutines without failsafes
    //inputs are key and initialization vector to use as well as ciphertext and plaintext variables as source or target respectively
    void decryptData(const CryptoPP::SecByteBlock &k1, const CryptoPP::SecByteBlock &iv, const CryptoPP::SecByteBlock &ciphertext, CryptoPP::SecByteBlock &rPlaintext);
    void encryptData(const CryptoPP::SecByteBlock &k1, const CryptoPP::SecByteBlock &iv, CryptoPP::SecByteBlock &rCiphertext, const CryptoPP::SecByteBlock &plaintext);

    //auxiliary functions
    //decrypt master key and save it to rKey. for that, do the following: password and salt yield intermediate "long" key K, calculated with calcK.
    //in verifyK2 the known array hk2 is compared to the sha1-hash of the second half of K for password verification
    //the remaining half of K is used to decrypt the master key, using known initialization vector iv.
    bool verifyPasswordAndDecryptMasterKey(const CryptoPP::SecByteBlock &password, const CryptoPP::SecByteBlock &salt, const CryptoPP::SecByteBlock &iv, const CryptoPP::SecByteBlock &hk2, const CryptoPP::SecByteBlock &encryptedKey, CryptoPP::SecByteBlock &rK);
    void calcK(const CryptoPP::SecByteBlock &password, const CryptoPP::SecByteBlock &salt, CryptoPP::SecByteBlock &rK1, CryptoPP::SecByteBlock &rK2);
    bool verifyK2(const CryptoPP::SecByteBlock &hk2, CryptoPP::SecByteBlock &k2);

};

#endif // CRYPTOBUFFER_H
