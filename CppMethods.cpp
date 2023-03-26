#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <ctime>
#include <Ipexport.h>

using namespace std;

// Link with iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "ws2_32.lib")

constexpr auto WorkingBufferSize = 15000;
constexpr auto MaxTries = 3;

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

std::string ToMacString(const unsigned char* mac, const int macAddrLen)
{
    std::string strMac;
    for (int i = 0; i < macAddrLen; ++i)
    {
        const auto high = mac[i] / 16;
        strMac += high > 9 ? 'A' + high - 10 : '0' + high;
        const auto low = mac[i] % 16;
        strMac += low > 9 ? 'A' + low - 10 : '0' + low;
    }

    return strMac;
}

std::string WString2String(const std::wstring& wStr)
{
    const auto len = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), wStr.size(), nullptr, 0, nullptr, nullptr);
    const auto buffer = new char[len + 1];
    memset(buffer, 0, sizeof(char) * (len + 1));
    WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), wStr.size(), buffer, len, nullptr, nullptr);
    std::string result(buffer);
    delete[] buffer;
    return result;
}

int Char2Wchar(wchar_t* wcharStr, const char* charStr) {
    const auto len = MultiByteToWideChar(CP_ACP, 0, charStr, strlen(charStr), nullptr, 0);

    MultiByteToWideChar(CP_ACP, 0, charStr, strlen(charStr), wcharStr, len);
    wcharStr[len] = '\0';
    return len;
}

void Adapter()
{
    /* Declare and initialize variables */

    DWORD dwRetVal;

    // Set the flags to pass to GetAdaptersAddresses
    constexpr ULONG flags = GAA_FLAG_INCLUDE_PREFIX //返回IPV4+IPV6
        | GAA_FLAG_SKIP_DNS_SERVER //不用返回DSN
        | GAA_FLAG_SKIP_MULTICAST //不用返回多播地址
        | GAA_FLAG_SKIP_ANYCAST;//不用返回广播地址

    // default to unspecified address family (both)
    constexpr ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = nullptr;

    PIP_ADAPTER_ADDRESSES pAddresses;
    ULONG iterations = 0;

    cout << "Calling GetAdaptersAddresses function with family = ";

    switch (family)
    {
    case AF_INET:
        cout << "AF_INET";
        break;
    case AF_INET6:
        cout << "AF_INET6";
        break;
    case AF_UNSPEC:
        cout << "AF_UNSPEC";
        break;
    default:
        return;
    }

    // Allocate a 15 KB buffer to start with.
    ULONG outBufLen = WorkingBufferSize;

    do {

        pAddresses = static_cast<IP_ADAPTER_ADDRESSES*>(MALLOC(outBufLen));
        if (pAddresses == nullptr) {
            cout << "Memory allocation failed for IP_ADAPTER_ADDRESSES struct" << endl;
            return;
        }

        dwRetVal = GetAdaptersAddresses(family, flags, nullptr, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = nullptr;
        }
        else
            break;

        iterations++;

    } while (dwRetVal == ERROR_BUFFER_OVERFLOW && iterations < MaxTries);

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {

            string adapterName = pCurrAddresses->AdapterName;
            cout << "adapter name: " + adapterName << endl;
            const auto adapterDescription = pCurrAddresses->Description;
            cout << "adapter description: " << WString2String(adapterDescription) << endl;

            auto macAddress = ToMacString(pCurrAddresses->PhysicalAddress, pCurrAddresses->PhysicalAddressLength);
            cout << "mac: " << macAddress << endl;
            auto pUnicast = pCurrAddresses->FirstUnicastAddress;
            int index = 1;
            while (pUnicast)
            {
                CHAR ip[130] = { 0 };
                switch (pUnicast->Address.lpSockaddr->sa_family)
                {
                    // IPV4 地址，使用 IPV4 转换
                case AF_INET:
                    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr)->sin_addr, ip, sizeof ip);
                    break;
                    // IPV6 地址，使用 IPV6 转换
                case AF_INET6:
                    inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(pUnicast->Address.lpSockaddr)->sin6_addr, ip, sizeof ip);
                    break;
                default:
                    break;
                }
                pUnicast = pUnicast->Next;
                cout << "ip address index " << index << ": " << ip << endl;
                ++index;
            }

            auto pAnycast = pCurrAddresses->FirstAnycastAddress;
            if (pAnycast) {
                auto i = 0;
                for (; pAnycast != nullptr; ++i)
                    pAnycast = pAnycast->Next;
                cout << "\tNumber of Anycast Addresses: " << i << endl;
            }
            else
                cout << "\tNo Anycast Addresses" << endl;



            cout << "\tIfType: " << pCurrAddresses->IfType << endl
                << "\tOperStatus: " << pCurrAddresses->OperStatus << endl
                << "\tIpv6IfIndex (IPv6 interface): " << pCurrAddresses->Ipv6IfIndex << endl
                << "\tZoneIndices (hex): ";
            for (const auto zoneIndice : pCurrAddresses->ZoneIndices)
                printf("%lx ", zoneIndice);
            cout << endl
                << "\tTransmit link speed: " << pCurrAddresses->TransmitLinkSpeed << endl
                << "\tReceive link speed: " << pCurrAddresses->ReceiveLinkSpeed << endl;

            const IP_ADAPTER_PREFIX* pPrefix = pCurrAddresses->FirstPrefix;
            cout << "\tNumber of IP Adapter Prefix entries: ";
            if (pPrefix) {
                auto i = 0;
                for (; pPrefix != nullptr; ++i)
                    pPrefix = pPrefix->Next;
                cout << i;
            }
            else
                cout << "0";

            cout << endl << endl;

            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    else {
        cout << "Call to GetAdaptersAddresses failed with error: " << dwRetVal << endl;
        if (dwRetVal == ERROR_NO_DATA)
            cout << "\tNo addresses were found for the requested parameters" << endl;
        else if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // Default language
            reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr)) {
            cout << "\tError: " << lpMsgBuf;
            LocalFree(lpMsgBuf);
            if (pAddresses)
                FREE(pAddresses);
            return;
        }
    }

    if (pAddresses)
        FREE(pAddresses);
}

bool IsLocalAdapter(char* pAdapterName)
{
    wchar_t netCardKey[MAX_PATH + 1];
    wsprintf(netCardKey, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%hs\\Connection", pAdapterName);
    wchar_t szDataBuf[MAX_PATH + 1];
    DWORD dwDataLen = MAX_PATH;
    DWORD dwType = REG_SZ;
    // if (ERROR_SUCCESS != RegGetValue(HKEY_LOCAL_MACHINE, NET_CARD_KEY, L"MediaSubType", RRF_RT_REG_DWORD, &dwType2, szDataBuf, &dwDataLen))
    //     return false;
    if (ERROR_SUCCESS != RegGetValue(HKEY_LOCAL_MACHINE, netCardKey, L"PnPInstanceId", RRF_RT_REG_SZ, &dwType, szDataBuf, &dwDataLen))
        return false;
    return static_cast<bool>(wcsstr(szDataBuf, L"PCI"));
}

void IpConfig()
{
    DWORD returnCheckError;
    DWORD fixedInfoSize = 0;

    PIP_ADDR_STRING pAddrStr;
    PIP_ADAPTER_INFO pAdapterInfo;

    if ((returnCheckError = GetNetworkParams(nullptr, &fixedInfoSize)) != 0)
        if (returnCheckError != ERROR_BUFFER_OVERFLOW)
        {
            cerr << "GetNetworkParams sizing failed with error" << returnCheckError << endl;
            return;
        }

    // Allocate memory from sizing information
    const auto pFixedInfo = static_cast<PFIXED_INFO>(malloc(fixedInfoSize));
    if (pFixedInfo == nullptr)
    {
        cerr << "Memory allocation error" << endl;
        return;
    }

    if ((returnCheckError = GetNetworkParams(pFixedInfo, &fixedInfoSize)) == 0)
    {
        cout << "\tHost Name . . . . . . . . . : " << pFixedInfo->HostName << endl
            << "\tDNS Servers . . . . . . . . : " << pFixedInfo->DnsServerList.IpAddress.String << endl;
        pAddrStr = pFixedInfo->DnsServerList.Next;
        while (pAddrStr)
        {
            printf("%52s\n", pAddrStr->IpAddress.String);
            pAddrStr = pAddrStr->Next;
        }

        cout << "\tNode Type . . . . . . . . . : ";
        switch (pFixedInfo->NodeType)
        {
        case BROADCAST_NODETYPE:
            cout << "Broadcast";
            break;
        case PEER_TO_PEER_NODETYPE:
            cout << "Peer to peer";
            break;
        case MIXED_NODETYPE:
            cout << "Mixed";
            break;
        case HYBRID_NODETYPE:
            cout << "Hybrid";
            break;
        default:
            break;
        }
        cout << endl
            << "\tNetBIOS Scope ID. . . . . . : " << pFixedInfo->ScopeId << endl
            << "\tIP Routing Enabled. . . . . : " << (pFixedInfo->EnableRouting ? "yes" : "no") << endl
            << "\tWINS Proxy Enabled. . . . . : " << (pFixedInfo->EnableProxy ? "yes" : "no") << endl
            << "\tNetBIOS Resolution Uses DNS : " << (pFixedInfo->EnableDns ? "yes" : "no") << endl;
    }
    else
    {
        cerr << "GetNetworkParams failed with error " << returnCheckError << endl;
        return;
    }

    // Enumerate all of the adapter specific information using the IP_ADAPTER_INFO structure.
    // Note:  IP_ADAPTER_INFO contains a linked list of adapter entries.

    ULONG adapterInfoSize = 0;
    if ((returnCheckError = GetAdaptersInfo(nullptr, &adapterInfoSize)) != 0)
        if (returnCheckError != ERROR_BUFFER_OVERFLOW)
        {
            cerr << "GetAdaptersInfo sizing failed with error " << returnCheckError << endl;
            return;
        }

    // Allocate memory from sizing information
    if ((pAdapterInfo = static_cast<PIP_ADAPTER_INFO>(GlobalAlloc(GPTR, adapterInfoSize))) == nullptr)
    {
        cerr << "Memory allocation error" << endl;
        return;
    }

    // Get actual adapter information
    if ((returnCheckError = GetAdaptersInfo(pAdapterInfo, &adapterInfoSize)) != 0)
    {
        cerr << "GetAdaptersInfo failed with error " << returnCheckError << endl;
        return;
    }

    PIP_ADAPTER_INFO pAdapt = pAdapterInfo;

    while (pAdapt)
    {
        cout << endl << endl;
        switch (pAdapt->Type)
        {
        case MIB_IF_TYPE_ETHERNET:
            cout << "Ethernet";
            break;
        case MIB_IF_TYPE_TOKENRING:
            cout << "Token Ring";
            break;
        case MIB_IF_TYPE_FDDI:
            cout << "FDDI";
            break;
        case MIB_IF_TYPE_PPP:
            cout << "PPP";
            break;
        case MIB_IF_TYPE_LOOPBACK:
            cout << "Loopback";
            break;
        case MIB_IF_TYPE_SLIP:
            cout << "Slip";
            break;
        case MIB_IF_TYPE_OTHER:
        default:
            cout << "Other";
        }
        cout << " adapter " << pAdapt->AdapterName
            << " (" << (IsLocalAdapter(pAdapt->AdapterName) ? "Physical" : "Virtual") << "):" << endl
            << "\tDescription . . . . . . . . : " << pAdapt->Description << endl
            << "\tPhysical Address. . . . . . : ";

        for (auto i = 0u; i < pAdapt->AddressLength; ++i)
            printf(i == pAdapt->AddressLength - 1 ? "%.2X\n" : "%.2X-", static_cast<int>(pAdapt->Address[i]));

        cout << "\tDHCP Enabled. . . . . . . . : " << (pAdapt->DhcpEnabled ? "yes" : "no") << endl;

        pAddrStr = &pAdapt->IpAddressList;
        while (pAddrStr)
        {
            cout << "\tIP Address. . . . . . . . . : " << pAddrStr->IpAddress.String << endl
                << "\tSubnet Mask . . . . . . . . : " << pAddrStr->IpMask.String << endl;
            pAddrStr = pAddrStr->Next;
        }

        cout << "\tDefault Gateway . . . . . . : " << pAdapt->GatewayList.IpAddress.String << endl;
        pAddrStr = pAdapt->GatewayList.Next;
        while (pAddrStr)
        {
            printf("%52s\n", pAddrStr->IpAddress.String);
            pAddrStr = pAddrStr->Next;
        }

        cout << "\tDHCP Server . . . . . . . . : " << pAdapt->DhcpServer.IpAddress.String << endl
            << "\tPrimary WINS Server . . . . : " << pAdapt->PrimaryWinsServer.IpAddress.String << endl
            << "\tSecondary WINS Server . . . : " << pAdapt->SecondaryWinsServer.IpAddress.String << endl;

        tm newTime{};

        char buffer[50] = { 0 };
        // Display coordinated universal time - GMT 
        auto _ = gmtime_s(&newTime, &pAdapt->LeaseObtained);
        cout << "\tLease Obtained. . . . . . . : " << asctime_s(buffer, &newTime) << endl;

        _ = gmtime_s(&newTime, &pAdapt->LeaseExpires);
        cout << "\tLease Expires . . . . . . . : " << asctime_s(buffer, &newTime);

        pAdapt = pAdapt->Next;
    }
}

int main(int argc, char* argv[])
{
    Adapter();
}
