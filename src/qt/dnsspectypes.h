#ifndef DNSSPECTYPES_H
#define DNSSPECTYPES_H

#include <QJsonObject>
#include <QString>

#define ADD_JSON_SERIALIZE_METHODS       \
    void read(const QJsonObject &json);  \
    void write(QJsonObject &json) const;

/**
 * Domain type, container for top level domains and mappable
 * subdomains
 *     type enum {topLevel, subDomain}
 *     name: QString (add role formatters)
 *     readJson, writeJson
 *
 * map to be implemented as nested Domain objects (under topLevel)
 *      "map": {
 *          "www" : { "alias": "" },
 *          "ftp" : { "ip": ["10.2.3.4", "10.4.3.2"] },
 *          "mail": { "ns": ["ns1.host.net", "ns12.host.net"] }
 *      }
 */
class Domain
{
public:
    enum DomainType {
        TopLevel, SubDomain
    };
    explicit Domain(const QString &name, DomainType type);

    const QString &getName() const;
    void setName(const QString &name);

    A &getA() const;
    CNAME &getCNAME() const;
    SOARP &getSOARP() const;
    DNAME &getDNAME() const;
    NS &getNS() const;
    DS &getDS() const;
    TLS &getTLS() const;
    SRV &getSRV() const;
    TXT &getTXT() const;
    IMPORT &getIMPORT() const;
    SSHFP &getSSHFP() const;

    // these set up recursive maps
    Domain &getSubdomain(const QString &name);
    void setSubdomain(const QString &name);

    ADD_JSON_SERIALIZE_METHODS;
private:
    A a;
    CNAME cname;
    SOARP soarp;
    DNAME dname;
    NS ns;
    DS ds;
    TLS tls;
    SRV srv;
    TXT txt;
    IMPORT import;
    SSHFP sshfp;
    QString name;
    // subdomainName -> Domain
    QMap<QString, Domain> subdomains;
};

/**
 * A/AAAA record.
 *   "ip"      : "192.168.1.1",
 *   "ip6"     : "2001:4860:0:1001::68",
 *   "tor"     : "eqt5g4fuenphqinx.onion",
 *   "i2p"     : // TODO: decide what to do with this (needs "specifier")
 *   "freenet" : "USK@0I8g...xbZ4,AQACAAE/Example/42/"
 */
class A
{
public:
    enum ResourceType {
        IPv4, IPv6, Tor, I2P, Freenet, ZeroNet
    };
    explicit A();
    explicit A(const QString &host, ResourceType type);

    const QString &getHost() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
};

/**
 * CNAME:
 *   "alias": "example.bit." // must be a valid dns name
 */
class CNAME
{
public:
    explicit CNAME();
    explicit CNAME(const QString &host);

    const QString &getHost() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
};

/**
 * SOA/RP:
 *   "email"   : "hostmaster@example.bit",
 *   "info"    : "Example & Sons Co."
 */
class SOARP
{
public:
    explicit SOARP();
    explicit SOARP(const QString &email, const QString &info);

    // TODO: email may be deprecated
    const QString &getEmail() const;
    void setEmail(const QString &email);

    const QString &getInfo() const;
    void setInfo(const QString &info);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString email;
    QString info;
};

/**
 * DNAME:
 *   "translate": "otherhost.bit."*
 */
class DNAME
{
public:
    explicit DNAME();
    explicit DNAME(const QString &host);

    const QString &getHost() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
};

/**
 * NS: [QList of strings]
 *   "ns": ["ns1.example.com.", "ns2.example.com."]
 *   "ns": "ns1.example.com."
 */
class NS
{
public:
    explicit NS();
    explicit NS(QList<QString> hosts);

    QList<QString> getHosts() const;
    void addHost(const QString &host);
    void removeHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> hosts;
};

/**
 * DS: [QList of 4d-vectors]
 *     "ds": [[12345,8,1,"EfatjsUqKYSrqv18O1FlA3hcIHI="],
 *            [12345,8,2,"LXEWQrcmsEQBYnyp+6wy9chTD7GQPMTbAiWHF5IaSIE="]]
 */
class DS
{
public:
    struct DSRecord
    {
        int keyTag;
        int algorithm;
        int hashType;
        QString hash;
    };

    explicit DS();
    explicit DS(QList<DSRecord> records);

    QList<DSRecord> getRecords() const;
    void setRecord(DSRecord record);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<DSRecord> records;
};

/**
 * TLS: [key-value map]
 *   "tls": {
 *     "tcp": {
 *       "443": [ [1, "660008F91...D7621B787", 1] ],
 *       "25": [ [1, "660008F91...D7621B787", 1] ]
 */
class TLS
{
public:
    struct TLSData
    {
        int certUsage;
        int selector;
        int matchingType;
        QString data;
    };

    typedef QMap<QString, QMap<int, TLSData>> TLSRecords;

    explicit TLS();
    explicit TLS(TLSRecords tlsRecords);

    TLSRecords getRecords() const;
    void setRecord(QString proto, int port, TLSData data);
    void removeRecord(QString proto, int port);

    ADD_JSON_SERIALIZE_METHODS;
private:
    TLSRecords records;
};

/**
 * SRV: [QList of 4d-vectors]
 *   "service" : [ [priorityInt, weightInt, portInt, dnsNameString], ...]
 */
class SRV
{
public:
    struct SRVRecord
    {
        int priority;
        int weight;
        int port;
        QString host;
    };

    typedef QList<SRVRecord> SRVRecords;

    explicit SRV();
    explicit SRV(SRVRecords records);

    SRVRecords getRecords() const;
    void addRecord(SRVRecord record);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    SRVRecords records;
};

/**
 * TXT:
 *   "txt": "string" // limit to 255 bytes
 */
class TXT
{
public:
    explicit TXT();
    explicit TXT(const QString &data);

    const QString &getData() const;
    void setData(const QString &data);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString data;
};

/**
 * IMPORT: [QList of strings]
 *     "import": ["dd/alpha", "dd/beta"]
 */
class IMPORT
{
public:
    explicit IMPORT();
    explicit IMPORT(QList<QString> imports);

    QList<QString> getImports() const;
    void addImport(const QString &import);
    void removeImport(const QString &import);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> imports;
};

/**
 * SSHFP: [QList of 3d-vectors]
 *     "sshfp": [[2,1,"EjRWeJq83vZ4kBI0VniavN72eJA="]]
 */
class SSHFP
{
public:
    struct SSHFPRecord
    {
        int algorithm;
        int fingerprintType;
        QString fingerprint;
    };

    explicit SSHFP();
    explicit SSHFP(QList<SSHFPRecord> fingerprints);

    QList<SSHFPRecord> getFingerprints() const;
    void addRecord(SSHFPRecord fingerprint);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<SSHFPRecord> fingerprints;
};

#endif // DNSSPECTYPES_H
