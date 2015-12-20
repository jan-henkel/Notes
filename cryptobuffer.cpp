#include "cryptobuffer.h"

CryptoInterface::CryptoInterface(QObject *parent) : QObject(parent), pbkdf2iterations_(DEFAULT_PBKDF2_ITERATIONS), keyIVSet_(false)
{
    QCA::init();
}

bool CryptoInterface::verifyPasswordAndDecryptMasterKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &k2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rKey)
{
    QCA::SymmetricKey k;
    //intermediate long key k from password and salt
    calcK(password,salt,k);
    //verify password, remove latter half of k if successful
    if(!verifyK2AndTruncateK(k2,k))
        return false;
    //decrypt master key using remaining half of k
    decryptData(k,iv,encryptedKey,rKey);
    return true;
}

void CryptoInterface::calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK)
{
    //derive long key K using pbkdf2(password,salt). second half of K is used as password verification, first half to encrypt / decrypt master key.
    QCA::PBKDF2 keyDerivationAlgorithm("sha1");
    rK=keyDerivationAlgorithm.makeKey(password,salt,64,10000); //length = 64 bytes = 512 bits
}

bool CryptoInterface::verifyK2AndTruncateK(const QCA::SecureArray &k2, QCA::SecureArray &k)
{
    //compare k2 to latter half of k for password verification (long key k is derived from password, the latter half remains is known)
    for(int i=32;i<64;++i)
    {
        if(k[i]!=k2[i-32])
            return false; //no match
    }
    //truncate
    k.resize(32);
    return true;
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

CryptoInterface::Result CryptoInterface::readKeyIV(const QCA::SecureArray &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    if(!file.exists())
        return NO_FILE;
    file.open(QFile::ReadOnly);

    //read initialization vector (unencrypted)
    QCA::SecureArray iv(32);
    if(!file.read(iv.data(),32))
        return WRONG_FILE_FORMAT;

    //read salt (unencrypted)
    QCA::SecureArray salt(32);
    if(!file.read(salt.data(),32))
        return WRONG_FILE_FORMAT;

    //read known latter half k2 of derived intermediate key
    //k2 is to be used for password verification
    QCA::SecureArray k2(32);
    if(!file.read(k2.data(),32))
        return WRONG_FILE_FORMAT;

    //read encrypted master key and encrypted default initialization vector. 48 bytes each due to padding.
    QCA::SecureArray encryptedKey(48);
    if(!file.read(encryptedKey.data(),48))
        return WRONG_FILE_FORMAT;
    QCA::SecureArray encryptedIV(48);
    if(!file.read(encryptedIV.data(),48))
        return WRONG_FILE_FORMAT;
    if(!file.atEnd())
        return WRONG_FILE_FORMAT;
    file.close();

    QCA::SymmetricKey k;
    //generate long intermediate key k from password, using salt
    calcK(password,salt,k);
    //verify password by comparing the second half of k to k2
    if(!verifyK2AndTruncateK(k2,k))
        return WRONG_PASSWORD;
    //if password is correct, decrypt the master key and default initialization vector
    decryptData(k,iv,encryptedKey,masterkey_);
    decryptData(k,iv,encryptedIV,defaultiv_);
    keyIVSet_=true;
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::writeKeyIV(const QCA::SecureArray &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    file.open(QFile::WriteOnly);

    //write random iv (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    QCA::InitializationVector iv(32);
    file.write(iv.data(),32);

    //write random salt (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    QCA::InitializationVector salt(32);
    file.write(salt.data(),32);

    //derive long intermediate key k using password and salt
    QCA::SymmetricKey k;
    calcK(password,salt,k);
    //write latter half of k to file unencrypted
    file.write(&k[32],32);

    //encrypt master key and default iv with first half of k and above iv
    k.resize(32);
    QCA::SymmetricKey encryptedKey;
    encryptData(k,iv,encryptedKey,masterkey_);
    file.write(encryptedKey.data(),48);
    QCA::SecureArray encryptedIV;
    encryptData(k,iv,encryptedIV,defaultiv_);
    file.write(encryptedIV.data(),48);
    file.close();
    return SUCCESS;
}

void CryptoInterface::setKeyIV(const QCA::SymmetricKey &key, const QCA::SymmetricKey &iv)
{
    masterkey_=key;
    defaultiv_=iv;
    keyIVSet_=true;
}

void CryptoInterface::setRandomKeyIV()
{
    masterkey_=QCA::InitializationVector(32);
    defaultiv_=QCA::InitializationVector(32);
    keyIVSet_=true;
}

CryptoInterface::Result CryptoInterface::encrypt(QCA::SecureArray &rCipher,const QCA::SecureArray &plain,const QCA::InitializationVector &iv)
{
    if(!keyIVSet_)
        return KEY_NOT_SET;
    encryptData(masterkey_,iv,rCipher,plain);
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::decrypt(const QCA::SecureArray &cipher, QCA::SecureArray &rPlain,const QCA::InitializationVector &iv)
{
    if(!keyIVSet_)
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

QCA::SecureArray CryptoInterface::encrypt(const QCA::SecureArray &plain)
{
    return encrypt(plain,defaultiv_);
}

QCA::SecureArray CryptoInterface::decrypt(const QCA::SecureArray &cipher, const QCA::InitializationVector &iv)
{
    QCA::SecureArray ret;
    decrypt(cipher,ret,iv);
    return ret;
}

QCA::SecureArray CryptoInterface::decrypt(const QCA::SecureArray &cipher)
{
    return decrypt(cipher,defaultiv_);
}

CryptoInterface::Result CryptoInterface::encrypt(QCA::SecureArray &rCipher,const QCA::SecureArray &plain)
{
    return encrypt(rCipher,plain,defaultiv_);
}

CryptoInterface::Result CryptoInterface::decrypt(const QCA::SecureArray &cipher, QCA::SecureArray &rPlain)
{
    return decrypt(cipher,rPlain,defaultiv_);
}

void CryptoInterface::scrambleMemory(QCA::SecureArray &memory, quint64 size)
{
    memory.fill(char(0),size-1);
    for(unsigned int i=0;i<size;++i)
        memory[i]=QCA::Random::randomChar();
}
