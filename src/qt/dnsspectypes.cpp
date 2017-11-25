#include "dnsspectypes.h"

#include <iostream> // std::cout

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
    a =      new A();
    cname =  new CNAME();
    soarp =  new SOARP();
    dname =  new DNAME();
    ns =     new NS();
    ds =     new DS();
    tls =    new TLS();
    srv =    new SRV();
    txt =    new TXT();
    import = new IMPORT();
    sshfp =  new SSHFP();
    // subdomainName -> Domain
    QMap<QString, Domain> subdomains = QMap<QString, Domain>();
};

bool Domain::load(const QString &data, QString *error)
{
    std::cout << "DATA " << data.toStdString() << '\n';
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data.toStdString().c_str()), &parseError);
    if (parseError.error == QJsonParseError::NoError)
    {
        std::cout << "Successful JSON parse!\n";
        return true;
    }
    else
        std::cout << "JSON ERROR " << parseError.errorString().toStdString() << '\n';
    QJsonObject json = doc.object();
    return false;
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
