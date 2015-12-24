#include "cryptointerface.h"

CryptoInterface::CryptoInterface(QObject *parent) : QObject(parent), pbkdf2iterations_(DEFAULT_PBKDF2_ITERATIONS), masterKeySet_(false)
{
    QCA::init();
    setEncryptionScheme(PBKDF2_SHA1_AES256);
}

bool CryptoInterface::verifyPasswordAndDecryptMasterKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &hk2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rK)
{
    QCA::SecureArray k1,k2;
    //intermediate long key k=(k1,k2) from password and salt
    calcK(password,salt,k1,k2);
    //verify password, remove latter half of k if successful
    if(!verifyK2(hk2,k2))
        return false;
    //decrypt master key using k1
    decryptData(k1,iv,encryptedKey,rK);
    return true;
}

void CryptoInterface::calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK1, QCA::SecureArray &rK2)
{
    //derive long key K using pbkdf2(password,salt). second half of K is used as password verification, first half to encrypt / decrypt master key.
    QCA::PBKDF2 keyDerivationAlgorithm("sha1");
    //default padding QCA::SecureArray(1,0) to allow for empty passwords
    QCA::SecureArray k=keyDerivationAlgorithm.makeKey(password+QCA::SecureArray(1,0),salt,64,pbkdf2iterations_); //length = 64 bytes = 512 bits
    rK1.resize(32);
    rK2.resize(32);
    for(int i=0;i<32;++i)
        {rK1[i]=k[i]; rK2[i]=k[i+32];}
}

bool CryptoInterface::verifyK2(const QCA::SecureArray &hk2, QCA::SecureArray &k2)
{
    if(hk2==QCA::Hash("sha1").hash(QCA::SecureArray(k2)))
        return true;
    else
        return false;
}

void CryptoInterface::decryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, const QCA::SecureArray &ciphertext, QCA::SecureArray &rPlaintext)
{
    //aes-decrypt ciphertext using key k1, initialization vector iv. save to rPlaintext
    QCA::Cipher cipher("aes256",QCA::Cipher::CBC,QCA::Cipher::DefaultPadding,QCA::Decode,k1,iv);
    rPlaintext=QCA::SecureArray(cipher.process(ciphertext));
}

void CryptoInterface::encryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, QCA::SecureArray &rCiphertext, const QCA::SecureArray &plaintext)
{
    //aes-encrypt plaintext using key k1, initialization vector iv. save to rCiphertext
    QCA::Cipher cipher("aes256",QCA::Cipher::CBC,QCA::Cipher::DefaultPadding,QCA::Encode,k1,iv);
    rCiphertext=QCA::SecureArray(cipher.process(plaintext));
}

CryptoInterface::Result CryptoInterface::readMasterKey(const QCA::SecureArray &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    if(!file.exists())
        return NO_FILE;
    file.open(QFile::ReadOnly);

    //read encryption scheme
    file.read((char*)&this->encryptionScheme_,sizeof(EncryptionScheme));

    //read initialization vector (unencrypted)
    QCA::SecureArray iv(16); //blocksize of AES is 128 bit regardless of keylength
    if(!file.read(iv.data(),16))
        return WRONG_FILE_FORMAT;

    //read salt (unencrypted)
    QCA::SecureArray salt(32);
    if(!file.read(salt.data(),32))
        return WRONG_FILE_FORMAT;

    //read sha1 hash of latter half k2 of derived intermediate key
    //to be used for password verification
    QCA::SecureArray hk2(20);
    if(!file.read(hk2.data(),20))
        return WRONG_FILE_FORMAT;

    //read encrypted master key. 48 bytes due to padding.
    QCA::SecureArray encryptedKey(48);
    if(!file.read(encryptedKey.data(),48))
        return WRONG_FILE_FORMAT;
    if(!file.atEnd())
        return WRONG_FILE_FORMAT;
    file.close();

    //generate long intermediate key k=(k1,k2) from password, using salt
    QCA::SecureArray k1;
    QCA::SecureArray k2;
    calcK(password,salt,k1,k2);

    //verify password by comparing the second half of k to k2
    if(!verifyK2(hk2,k2))
        return WRONG_PASSWORD;
    //if password is correct, decrypt the master key
    decryptData(k1,iv,encryptedKey,masterkey_);
    masterKeySet_=true;
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::saveMasterKey(const QCA::SecureArray &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    file.open(QFile::WriteOnly);

    //write encryption scheme
    file.write((char*)&this->encryptionScheme_,sizeof(EncryptionScheme));

    //write random iv (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    QCA::InitializationVector iv(16);
    iv=QCA::Random::randomArray(16);
    file.write(iv.data(),16);

    //write random salt (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    QCA::InitializationVector salt(32);
    salt=QCA::Random::randomArray(32);
    file.write(salt.data(),32);

    //derive long intermediate key k using password and salt
    QCA::SecureArray k1;
    QCA::SecureArray k2;
    calcK(password,salt,k1,k2);

    //write latter half of k to file unencrypted
    file.write(QCA::Hash("sha1").hash(k2).toByteArray());

    //encrypt master key with first half of k and initialization vector
    QCA::SymmetricKey encryptedKey;
    encryptData(k1,iv,encryptedKey,masterkey_);
    file.write(encryptedKey.data(),48);
    file.close();
    return SUCCESS;
}

void CryptoInterface::setMasterKey(const QCA::SymmetricKey &key)
{
    masterkey_=key;
    masterKeySet_=true;
}

void CryptoInterface::setRandomMasterKey()
{
    masterkey_=QCA::SymmetricKey(32);
    masterKeySet_=true;
}

CryptoInterface::Result CryptoInterface::encrypt(QCA::SecureArray &rCipher,const QCA::SecureArray &plain,const QCA::InitializationVector &iv)
{
    if(!masterKeySet_)
        return KEY_NOT_SET;
    encryptData(masterkey_,iv,rCipher,plain);
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::decrypt(const QCA::SecureArray &cipher, QCA::SecureArray &rPlain,const QCA::InitializationVector &iv)
{
    if(!masterKeySet_)
        return KEY_NOT_SET;
    decryptData(masterkey_,iv,cipher,rPlain);
    return SUCCESS;
}

QCA::SecureArray CryptoInterface::encrypt(const QCA::SecureArray &plain, const QCA::InitializationVector &iv)
{
    QCA::SecureArray ret;
    encrypt(ret,plain,iv);
    return ret;
}

QCA::SecureArray CryptoInterface::decrypt(const QCA::SecureArray &cipher, const QCA::InitializationVector &iv)
{
    QCA::SecureArray ret;
    decrypt(cipher,ret,iv);
    return ret;
}

void CryptoInterface::scrambleMemory(QCA::SecureArray &memory, quint64 size)
{
    memory.fill(char(0),size-1);
    for(unsigned int i=0;i<size;++i)
        memory[i]=QCA::Random::randomChar();
}
