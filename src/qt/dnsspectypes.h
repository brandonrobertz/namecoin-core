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

    QString name() const;
    void setName(const QString &name);

    const A &a() const;
    const CNAME &cname() const;
    const SOARP &soarp() const;
    const DNAME &dname() const;
    const NS &ns() const;
    const DS &ds() const;
    const TLS &tls() const;
    const SRV &srv() const;
    const TXT &txt() const;
    const IMPORT &import() const;
    const SSHFP &sshfp() const;

    ADD_JSON_SERIALIZE_METHODS;
private:
    A _a;
    CNAME _cname;
    SOARP _soarp;
    DNAME _dname;
    NS _ns;
    DS _ds;
    TLS _tls;
    SRV _srv;
    TXT _txt;
    IMPORT _import;
    SSHFP _sshfp;
    QString _name;
}

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
        IPv4, IPv6, Tor, I2P, Freenet
    };
    explicit A();
    explicit A(const QString &host, ResourceType type);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
}

/**
 * CNAME:
 *   "alias": "example.bit." // must be a valid dns name
 */
class CNAME
{
public:
    explicit CNAME();
    explicit CNAME(const QString &host);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString _host;
}

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

    QString email() const;
    void setEmail(const QString &email);

    QString info() const;
    void setInfo(const QString &info);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString _email;
    QString _info;
}

/**
 * DNAME:
 *   "translate": "otherhost.bit."*
 */
class DNAME
{
public:
    explicit DNAME();
    explicit DNAME(const QString &host);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString _host;
}

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

    QList<QString> hosts() const;
    void addHost(const QString &host);
    void removeHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> _hosts;
}

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

    QList<DSRecord> records() const;
    void addRecord(DSRecord record);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<DSRecord> _records;
}

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
    explicit TLS();
    explicit TLS(QMap tlsRecords);

    QMap tlsRecords() const;

    ADD_JSON_SERIALIZE_METHODS;
private:
    QMap _tlsRecords;
}

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

    explicit SRV();
    explicit SRV(QList<SRVRecord> records);

    QList<SRVRecord> records() const;
    void addRecord(SRVRecord record);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<SRVRecord> _records;
}

/**
 * TXT:
 *   "txt": "string" // limit to 255 bytes
 */
class TXT
{
public:
    explicit TXT();
    explicit TXT(const QString &data);

    QString data() const;
    void setData(const QString &data);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString _data;
}

/**
 * IMPORT: [QList of strings]
 *     "import": ["dd/alpha", "dd/beta"]
 */
class IMPORT
{
public:
    explicit IMPORT();
    explicit IMPORT(QList<QString> imports);

    QList<QString> imports() const;
    void setImport(const QString &import);
    void removeImport(const QString &import);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> _imports;
}

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

    QList<SSHFPRecord> fingerprints() const;
    void addRecord(SSHFPRecord fingerprint);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<SSHFPRecord> _fingerprints;
}

#endif // DNSSPECTYPES_H
