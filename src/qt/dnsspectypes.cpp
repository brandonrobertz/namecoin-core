#include "dnsspectypes.h"

#include <iostream>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

A::A() {};
A::A(const QString &_host, ResourceType _type) : host(_host), type(_type) {};

CNAME::CNAME() {};
CNAME::CNAME(const QString &_host) : host(_host) {};

SOARP::SOARP() {};
SOARP::SOARP(const QString &_email, const QString &_info) : email(_email), info(_info) {};

DNAME::DNAME() {};
DNAME::DNAME(const QString &_host) : host(_host) {};

NS::NS() {};
NS::NS(QList<QString> _hosts) : hosts(_hosts) {};

DS::DS() {};
DS::DS(QList<DSRecord> _records) : records(_records) {};

TLS::TLS() {};
TLS::TLS(TLSRecords _tlsRecords) : tlsRecords(_tlsRecords) {};

SRV::SRV() {};
SRV::SRV(SRVRecords _records) : records(_records) {};

TXT::TXT() {};
TXT::TXT(const QString &_data) : data(_data) {};

IMPORT::IMPORT() {};
IMPORT::IMPORT(QList<QString> _imports) : imports(_imports) {};

SSHFP::SSHFP() {};
SSHFP::SSHFP(QList<SSHFPRecord> _fingerprints) : fingerprints(_fingerprints) {};

Domain::Domain(const QString &_name, DomainType _type) :
    name(_name), type(_type)
{
    a = A();
    cname = CNAME();
    soarp = SOARP();
    dname = DNAME();
    ns = NS();
    ds = DS();
    tls = TLS();
    srv = SRV();
    txt = TXT();
    import = IMPORT();
    sshfp = SSHFP();
    // subdomainName -> Domain
    QMap<QString, Domain> subdomains = QMap<QString, Domain>();
};

void Domain::load(const QString &data)
{
    std::cout << "DATA " << data.toStdString() << '\n';
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.toStdString().c_str()), &error);
    if (error.error == QJsonParseError::NoError)
        std::cout << "Successful JSON parse!\n";
    else
        std::cout << "JSON ERROR " << error.errorString().toStdString() << '\n';
    QJsonObject json = doc.object();
}

// Domain::read(const QJsonObject &json)
// {
//     for(const auto &key : json.const_iterator())
//     {
//         switch(key)
//         {
//         case("ip"):
//         case("ip6"):
//         case("tor"):
//         case("i2p"):
//         case("freenet"):
//         case("zeronet"):
//             a.read(json[key].toObject());
//             break;
//         case("map"):
//             subdomains.read(json[key].toObject());
//             break;
//         default:
//             assert(false);
//         }
//     }
// }
