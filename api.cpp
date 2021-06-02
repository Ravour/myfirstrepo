#include "api.h"
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
using namespace pcpp;

#define DATE_LEN 128

int convert_ASN1TIME(ASN1_TIME *t, char *buf, size_t len)
{
	int rc;
	BIO *b = BIO_new(BIO_s_mem());
	rc = ASN1_TIME_print(b, t);
	if (rc <= 0)
	{
		printf("ASN1_TIME_print failed or wrote no data.\n");
		BIO_free(b);
		return EXIT_FAILURE;
	}
	rc = BIO_gets(b, buf, len);
	if (rc <= 0)
	{
		printf("BIO_gets call failed to transfer contents to buf");
		BIO_free(b);
		return EXIT_FAILURE;
	}
	BIO_free(b);
	return EXIT_SUCCESS;
}

std::string getProtocolTypeAsString(pcpp::ProtocolType protocolType)
{
	switch (protocolType)
	{
	case pcpp::Ethernet:
		return "Ethernet";
	case pcpp::IPv4:
		return "IPv4";
	case pcpp::TCP:
		return "TCP";
	case pcpp::HTTPRequest:
	case pcpp::HTTPResponse:
		return "HTTP";
	default:
		return "Unknown";
	}
}

/**
 * An auxiliary method for sorting the uint16_t count map. Used in printPorts()
 */
bool uint16CountComparer(std::pair<uint16_t, int> first, std::pair<uint16_t, int> second)
{
	if (first.second == second.second)
	{
		return first.first > second.first;
	}
	return first.second > second.second;
}

bool stringCountComparer(std::pair<std::string, int> first, std::pair<std::string, int> second)
{
	if (first.second == second.second)
	{
		return first.first > second.first;
	}
	return first.second > second.second;
}

void printInfo(std::string pf, pcap_flow_feature &pff)
{
	PcapFileReaderDevice reader(pf);
	if (!reader.open())
	{
		printf("Error opening the pcap file\n");
		return;
	}
	RawPacket rawPacket;

	int first = true;
	pcpp::Layer *firstLayer;
	IPv4Layer *ip4Layer;
	int verEnum;
	// SSLStatsCollector collector;
	while (reader.getNextPacket(rawPacket))
	{
		pff.ts.push_back(rawPacket.getPacketTimeStamp());
		// std::cout << "timestamp:" << rawPacket.getPacketTimeStamp().tv_sec << "." << rawPacket.getPacketTimeStamp().tv_nsec << std::endl;
		Packet parsedPacket(&rawPacket);

		// /* 收集SSL层的信息，后续统计 */
		// collector->collectStats(&parsedPacket);

		firstLayer = parsedPacket.getFirstLayer();
		pff.pktLen.push_back((int)firstLayer->getDataLen());
		/* 源-目的IP */
		if (parsedPacket.isPacketOfType(pcpp::IPv4))
		{
			ip4Layer = parsedPacket.getLayerOfType<IPv4Layer>();
			pff.pktPayload.push_back((int)ip4Layer->getLayerPayloadSize());
			if (first)
			{
				pff.srcIP = ip4Layer->getSrcIPv4Address().toString();
				pff.dstIP = ip4Layer->getDstIPv4Address().toString();
				pff.pktCntUp++;
				pff.isUp.push_back(1);
				first = false;
			}
			else
			{
				if (parsedPacket.getLayerOfType<IPv4Layer>()->getSrcIPv4Address().toString() == pff.srcIP)
				{
					pff.pktCntUp++;
					pff.isUp.push_back(1);
				}

				else
				{
					pff.pktCntDown++;
					pff.isUp.push_back(0);
				}
			}
		}
		if (parsedPacket.isPacketOfType(pcpp::TCP))
		{
			pff.srcPort.push_back(parsedPacket.getLayerOfType<pcpp::TcpLayer>()->getSrcPort());
			pff.dstPort.push_back(parsedPacket.getLayerOfType<pcpp::TcpLayer>()->getDstPort());
		}

		/* encrypted alert 数据包 */
		if (!parsedPacket.isPacketOfType(pcpp::SSL))
		{
			// std::cout << "no ssl info,exit\n";
			continue;
		}
		else
		{
			pcpp::SSLHandshakeLayer *handshakeLayer = parsedPacket.getLayerOfType<pcpp::SSLHandshakeLayer>();
			if (handshakeLayer)
			{
				/* server hello message */
				pcpp::SSLServerHelloMessage *serverHelloMessage = handshakeLayer->getHandshakeMessageOfType<pcpp::SSLServerHelloMessage>();
				verEnum = handshakeLayer->getRecordVersion().asEnum();
				// std::cout << verEnum<<std::endl;
				pff.sslVer.push_back(verEnum);
				if (serverHelloMessage)
				{
					pff.serverCiph.push_back(serverHelloMessage->getCipherSuite()->asString());
					pff.serverHelloExtCnt = serverHelloMessage->getExtensionCount();
				}
				/* client hello message */
				pcpp::SSLClientHelloMessage *clientHelloMessage = handshakeLayer->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>();
				if (clientHelloMessage)
				{
					pff.clientHelloExtCnt = clientHelloMessage->getCipherSuiteCount();
					pff.clientHelloPktCnt++;
				}
				pcpp::SSLCertificateMessage *certMsg = handshakeLayer->getHandshakeMessageOfType<pcpp::SSLCertificateMessage>();
				if (certMsg)
				{
					int certCnt = certMsg->getNumOfCertificates();
					std::cout << "cert count: " << certCnt << std::endl;
					for (int i = 0; i < certCnt; i++)
					{
						pcpp::SSLx509Certificate *cert = certMsg->getCertificate(i);
						// std::string certBuffer(cert->getData(), cert->getData() + cert->getDataLength());
						// std::cout << certBuffer << std::endl;
						std::cout << "cert " << i << "  " << cert->getDataLength() << std::endl;
						uint8_t *certData = cert->getData();

						// for (size_t i = 0; i < cert->getDataLength(); i++)
						// {
						// 	printf("%x", certData[i]);
						// 	// std::cout << std::hex << certData[i];
						// }
						// std::cout << std::endl;
						X509 *parsedCert = d2i_X509(NULL, &certData, (long)cert->getDataLength());
						// printf("%p\n", cert);
						if (parsedCert)
						{
							X509_NAME *issuename = X509_get_issuer_name(parsedCert);
							if (issuename)
								printf("Issuer: %s\n", X509_NAME_oneline(issuename, NULL, 0));

							X509_NAME *subjectname = X509_get_subject_name(parsedCert);
							if (subjectname)
								printf("subject: %s\n", X509_NAME_oneline(subjectname, NULL, 0));

							/* 逐个输出以上信息 */

							for (int i = 0; i < X509_NAME_entry_count(subjectname); i++)
							{
								
								X509_NAME_ENTRY *e = X509_NAME_get_entry(subjectname, i);
								char objname[DATE_LEN];
								OBJ_obj2txt(objname, DATE_LEN, e->object, 0);
								// char *nstr = ASN1_STRING_data(e->value);
								printf("%s\t", objname);
								ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
								char *str = ASN1_STRING_data(d);
								printf("%s\n", str);
							}
							// printf("%s\n",subj->cn);
							// std::cout << "verify: " << X509_verify(parsedCert, X509_get_pubkey(parsedCert)) << std::endl;

							// if (!X509_NAME_cmp(issuename, subjectname))
							// {
							// 	std::cout << "issue name = subject name \n";
							// }
							// else
							// {
							// 	std::cout << "issue name != subject name \n";
							// }
							ASN1_TIME *not_before = X509_get_notBefore(parsedCert);
							char not_before_str[DATE_LEN];
							convert_ASN1TIME(not_before, not_before_str, DATE_LEN);
							printf("start: %s\n", not_before_str);

							ASN1_TIME *not_after = X509_get_notAfter(parsedCert);
							char not_after_str[DATE_LEN];
							convert_ASN1TIME(not_after, not_after_str, DATE_LEN);
							printf("expiration: %s\n", not_after_str);
							convert_ASN1TIME(not_before, not_before_str, DATE_LEN);

							int day, sec;
							if (ASN1_TIME_diff(&day, &sec, not_before, not_after))
							{
								std::cout << "period: " << day << "d " << sec << "s" << std::endl;
							}

							/* 版本 */
							// int version = ((int)X509_get_version(parsedCert)) + 1;
							// std::cout << version << std::endl;
							int raw = X509_check_ca(parsedCert);
							std::cout << "is CA certificate?: " << raw << std::endl;
							X509_free(parsedCert);
						}
					}
				}
				pcpp::SSLAlertLayer *alertLayer = parsedPacket.getLayerOfType<pcpp::SSLAlertLayer>();
				if (alertLayer)
				{
					pff.alertCnt++;
				}
			}
		}
	}
	pff.pktCnt = pff.pktCntDown + pff.pktCntUp;
	reader.close();
	return;
}