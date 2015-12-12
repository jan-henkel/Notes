#ifndef CRYPTOBUFFER_H
#define CRYPTOBUFFER_H

#include <QObject>
#include <Qca-qt5/QtCrypto/QtCrypto>
#include <QFile>

class CryptoBuffer : public QObject
{
    Q_OBJECT
public:
    enum Result {SUCCESS,WRONG_PASSWORD,NO_FILE,WRONG_FILE_FORMAT,KEY_NOT_SET};
    explicit CryptoBuffer(QObject *parent = 0);
    Result readKeyIV(const QCA::SecureArray &password, QString fileName);
    Result writeKeyIV(const QCA::SecureArray &password, QString fileName);
    void setKeyIV(const QCA::SymmetricKey &key,const QCA::SymmetricKey &iv);
    void setRandomKeyIV();
    Result encrypt(QCA::SecureArray &rCipher, const QCA::SecureArray &plain);
    Result decrypt(const QCA::SecureArray &cipher,QCA::SecureArray &rPlain);
    Result encrypt(QCA::SecureArray &rCipher, const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    Result decrypt(const QCA::SecureArray &cipher,QCA::SecureArray &rPlain,const QCA::InitializationVector &iv);
    QCA::SecureArray encrypt(const QCA::SecureArray &plain,const QCA::InitializationVector &iv);
    QCA::SecureArray decrypt(const QCA::SecureArray &cipher,const QCA::InitializationVector &iv);
    QCA::SecureArray encrypt(const QCA::SecureArray &plain);
    QCA::SecureArray decrypt(const QCA::SecureArray &cipher);
    void unsetKeyIV();
    bool keyIVSet() {return keyIVSet_;}
    void scrambleMemory(QCA::SecureArray &memory,quint64 size);
signals:

public slots:
private:
    QCA::SymmetricKey key_;
    QCA::InitializationVector iv_;
    bool keyIVSet_;
    bool calcKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &k2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rKey);
    void calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK);
    bool verifyK2AndTruncateK(const QCA::SecureArray &k2,QCA::SecureArray &k);
    void decryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, const QCA::SecureArray &ciphertext, QCA::SecureArray &rPlaintext);
    void encryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, QCA::SecureArray &rCiphertext, const QCA::SecureArray &plaintext);

};

#endif // CRYPTOBUFFER_H
