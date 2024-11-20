#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

using namespace std;

int main()
{
    HANDLE hFull, hEmpty, hEvent;
    STARTUPINFO* si;
    PROCESS_INFORMATION* pi;
    wchar_t fileName[50];

    wstring fileN;
    int recNum, receiverNum;
    cout << "Print file name and number of records: " << endl;
    wcin >> fileN >> recNum;
    if (recNum < 1) {
        cout << "Incorrect number of records.";
        return 1;
    }
    wstring s = L"Sender.exe " + fileN;
    lstrcpyW(fileName, s.data());

    ofstream createBinFile(fileN, ios::binary);
    createBinFile.close();

    // Создаем события
    hEvent = CreateEvent(NULL, FALSE, TRUE, L"SyncEvent");  // Автоматически сбрасываемое событие
    hFull = CreateSemaphore(NULL, 0, recNum, L"FullSemaphore");
    hEmpty = CreateSemaphore(NULL, recNum, recNum, L"EmptySemaphore");

    if (hEvent == NULL || hFull == NULL || hEmpty == NULL) {
        cout << "Error creating synchronization objects." << endl;
        return GetLastError();
    }

    cout << "Print number of receivers: " << endl;
    cin >> receiverNum;
    if (receiverNum < 1) {
        cout << "Incorrect number of receivers.";
        return 1;
    }

    si = new STARTUPINFO[receiverNum];
    pi = new PROCESS_INFORMATION[receiverNum];
    HANDLE* hProcessors = new HANDLE[receiverNum];
    ZeroMemory(si, sizeof(STARTUPINFO) * receiverNum);
    for (int i = 0; i < receiverNum; i++) {
        si[i].cb = sizeof(STARTUPINFO);
        if (!CreateProcess(NULL, fileName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si[i], &pi[i])) {
            cout << "Process " << i << " error";
            return GetLastError();
        }
        hProcessors[i] = pi->hProcess;
    }
    Sleep(100);

    if (receiverNum == 1)
        WaitForSingleObject(&hProcessors, INFINITE);
    else
        WaitForMultipleObjects(receiverNum, hProcessors, TRUE, INFINITE);

    ifstream inBinFile;
    ofstream outBinFile;
    string userAns;
    char* fileElem = new char[20];
    string deleting;

    while (true) {
        cout << "type your operation (exit, read):" << endl;
        cin >> userAns;

        if (userAns == "read") {
            WaitForSingleObject(hFull, INFINITE);
            WaitForSingleObject(hEvent, INFINITE); // Ожидание события

            inBinFile.open(fileN, ios::binary);
            inBinFile.read(fileElem, 20);
            inBinFile >> deleting;
            inBinFile.close();

            if (strcmp(fileElem, deleting.data()) == 0)
                deleting = "";

            outBinFile.open(fileN, ios::binary);
            outBinFile.clear();
            outBinFile.write(deleting.data(), deleting.size());
            outBinFile.close();

            cout << fileElem << endl;

            SetEvent(hEvent); // Устанавливаем событие
            ReleaseSemaphore(hEmpty, 1, NULL);
        }
        if (userAns == "exit") {
            for (int i = 0; i < receiverNum; i++) {
                TerminateProcess(pi[i].hProcess, 1);
                TerminateProcess(pi[i].hThread, 1);
            }
            break;
        }
        if (userAns != "read" && userAns != "exit") {
            cout << "Wrong command." << endl;
        }
    }

    WaitForMultipleObjects(receiverNum, &pi->hProcess, TRUE, INFINITE);

    for (int i = 0; i < receiverNum; i++) {
        CloseHandle(pi[i].hThread);
        CloseHandle(pi[i].hProcess);
    }

    CloseHandle(hEvent);
    CloseHandle(hFull);
    CloseHandle(hEmpty);
    inBinFile.close();
}
