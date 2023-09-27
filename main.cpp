#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

void GetProcessPorts(DWORD pid) {
    MIB_TCPTABLE_OWNER_PID* pTcpTable;
    DWORD dwSize = 0;

    // Get the size of the TCP table
    if (GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER) {
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
        if (pTcpTable == NULL) {
            std::cout << "Erro ao alocar memÃ³ria para a tabela TCP\n";
            return;
        }
    } else {
        std::cout << "Erro ao obter o tamanho da tabela TCP\n";
        return;
    }

    // Get the TCP table
    if (GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        //std::cout << "------------------------------------------------------------\n";
        std::cout << "Portas usadas pelo processo com PID: " << pid << "\n";
        //std::cout << "------------------------------------------------------------\n";
        std::cout << std::left << std::setw(10) << "Porta";
        std::cout << std::left << std::setw(25) << "Status";
        std::cout << "PID\n\n";
        //std::cout << "------------------------------------------------------------\n";

        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            MIB_TCPROW_OWNER_PID tcpRow = pTcpTable->table[i];
            if (tcpRow.dwOwningPid == pid) {
                std::cout << std::left << std::setw(10) << ntohs((u_short)tcpRow.dwLocalPort);
                std::cout << std::left << std::setw(25) << (tcpRow.dwState == MIB_TCP_STATE_ESTAB ? "Estabelecido" : "Fechado");
                std::cout << pid << "\n";
            }
        }
    } else {
        std::cout << "Erro ao obter a tabela TCP\n";
    }

    free(pTcpTable);
}

int main() {
    while (true) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            std::cout << "Erro ao criar snapshot do processo\n";
            return 1;
        }

        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &processEntry)) {
            std::cout << "Erro ao obter a primeira entrada do processo\n";
            CloseHandle(hSnapshot);
            return 1;
        }

        // Print header
        std::cout << std::left << std::setw(10) << "PID";
        std::cout << std::left << std::setw(25) << "Nome do Processo";
        std::cout << "Caminho do Processo\n";
        std::cout << "------------------------------------------------------------\n";

        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processEntry.th32ProcessID);
            if (hProcess != NULL) {
                char path[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH) != 0) {
                    std::cout << "=====================================================\n";
                    std::cout << std::left << std::setw(10) << processEntry.th32ProcessID;
                    std::cout << std::left << std::setw(25) << processEntry.szExeFile;
                    std::cout << path << "\n";

                    // Get ports used by the process
                    GetProcessPorts(processEntry.th32ProcessID);
                }
                CloseHandle(hProcess);
            }

            // Print child processes
            PROCESSENTRY32 childEntry;
            childEntry.dwSize = sizeof(PROCESSENTRY32);
            if (processEntry.th32ParentProcessID != 0) {
                HANDLE hChildSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (hChildSnapshot != INVALID_HANDLE_VALUE) {
                    if (Process32First(hChildSnapshot, &childEntry)) {
                        do {
                            if (childEntry.th32ParentProcessID == processEntry.th32ProcessID) {
                                HANDLE hChildProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, childEntry.th32ProcessID);
                                if (hChildProcess != NULL) {
                                    char childPath[MAX_PATH];
                                    if (GetModuleFileNameExA(hChildProcess, NULL, childPath, MAX_PATH) != 0) {
                                        std::cout << "-- PPID ";
                                        std::cout << std::left << std::setw(10) << childEntry.th32ProcessID;
                                        std::cout << std::left << std::setw(25) << childEntry.szExeFile;
                                        std::cout << childPath << "\n";

                                        // Get ports used by the child process
                                        GetProcessPorts(childEntry.th32ProcessID);
                                    }
                                    CloseHandle(hChildProcess);
                                }
                            }
                        } while (Process32Next(hChildSnapshot, &childEntry));
                    }
                    CloseHandle(hChildSnapshot);
                }
            }
        } while (Process32Next(hSnapshot, &processEntry));

        CloseHandle(hSnapshot);

     
        Sleep(5 * 60 * 1000); // Delay of 5 minutes
    }

    return 0;
}