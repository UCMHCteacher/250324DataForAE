#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <stdbool.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#endif


#ifdef _WIN32
#define CLOSE_SOCKET(s) closesocket(s)
#else
#define SOCKET int
#define CLOSE_SOCKET(s) close(s)
#endif


#include <cstdlib>



void WinSockSetup() {
#ifdef _WIN32
    BYTE FirstVersion=0x02;
    BYTE SecondVersion=0x02;
    WORD RequestVersion=MAKEWORD(FirstVersion,SecondVersion);

    WSADATA StartupMessageGot;
    int StartupCondition;
    StartupCondition=WSAStartup(RequestVersion,&StartupMessageGot);

    if(StartupCondition!=0)
    {
        printf("Network startup failed!\n");
        printf("Error Code:%d\n",StartupCondition);
        std::exit(EXIT_FAILURE);
    }

    printf("Using socket version %d.%d\n",
            LOBYTE(StartupMessageGot.wVersion),
            HIBYTE(StartupMessageGot.wVersion)
    );


    std::atexit(
        []{
            if(WSACleanup()!=0)
            {
                printf("Failed when closing network.\n");
                printf("Error Code:%d\n",WSAGetLastError());
            }
            printf("Successfully closed network.");
        }
    );
#endif
}







#include <thread>
#include <filesystem>
namespace stdfs = std::filesystem;
#include <iostream>
#include <sstream>
#include <fstream>
#include <future>
#include <chrono>

using namespace std::chrono_literals;





#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <cstdint>

sql::mysql::MySQL_Driver* driver = nullptr;
void SQLInit() {
    driver = sql::mysql::get_driver_instance();
}
sql::Connection* getConnection (std::string DBPassword) {
    sql::Connection* con = driver->connect(
        "tcp://127.0.0.1:3306",
        "root",
        DBPassword
    );

    con->setSchema("chunk");

    return con;
}









std::string FormatCommand(
    std::string networkDeviceIP,
    int chunkerWindowWidth,
    int chunkerThreadnum,
    int chunkProcessThreadnum,
    std::string summaryOutputPath,
    std::string DBPassword
) {
    std::ostringstream oss;
    oss 
#ifdef _WIN32
        << ".\\AEChunkProgram.exe "
#elif __linux__
        << "./AEChunkProgram "
#endif
        << "SourceType=Network " 
        << "NetworkDeviceIP=" << networkDeviceIP << ' '
        << "ChunkerWindowWidth=" << chunkerWindowWidth << ' '
        << "ChunkerThreadnum=" << chunkerThreadnum << ' '
        << "ChunkProcessorActionMode=CompareWithDataBase "
        << "ChunkProcessThreadnum=" << chunkProcessThreadnum << ' '
        << "SummaryOutputPath=" << summaryOutputPath << ' '
        << "DBPassword=" << DBPassword;
    return oss.str();
}


std::string FormatSummaryDirPath(
    int chunkerWindowWidth, 
    int chunkerThreadnum,
    int chunkProcessThreadnum
) {
    std::ostringstream oss;
    oss << "./Summary/Compare/w" << chunkerWindowWidth << "/j" << chunkerThreadnum << "." << chunkProcessThreadnum;
    return oss.str();
}



stdfs::path FormatSummaryOutputPath(stdfs::path summaryDirPath, int summaryCount) {
    std::ostringstream oss;
    oss << summaryCount << ".json";
    return summaryDirPath / oss.str();
}






stdfs::path terminateSymbol {"./DaCoda"};




/**
 * order of argc:
 * 
 * 1) chunkerWindowWidth
 * 2) chunkerThreadnum
 * 3) chunkProcessThreadnum
 * 
 * 4) networkDeviceIP
 * 
 * 5) DBPassword
 * 
 * 6) ServerIP
 */
int main(int argc, char* argv[]) {
    WinSockSetup();
    SQLInit();



    // check dataset
    stdfs::path datasetPath {"./Dataset"};
    if (!stdfs::is_directory(datasetPath)) {
        std::cout << "Dataset " << datasetPath << " doesn't exist!\n";
        std::exit(EXIT_FAILURE);
    }


    // get console args
    int chunkerWindowWidth = atoi(argv[1]);
    int chunkerThreadnum = atoi(argv[2]);
    int chunkProcessThreadnum = atoi(argv[3]);
    std::string networkDeviceIP {argv[4]};
    std::string DBPassword {argv[5]};
    std::string ServerIP {argv[6]};



    stdfs::path summaryDirPath = FormatSummaryDirPath(chunkerWindowWidth, chunkerThreadnum, chunkProcessThreadnum);
    if (!stdfs::is_directory(summaryDirPath)) {
        stdfs::create_directories(summaryDirPath);
    }




    // Terminate Symbol to end all sub processes

    std::atexit([]{
        stdfs::create_directories(terminateSymbol);
    });




    int summaryCount = 0;


    for (auto& dirIt : stdfs::directory_iterator{datasetPath}) {
        stdfs::remove(terminateSymbol);


        if (!dirIt.is_regular_file()) {
            continue;
        }
        stdfs::path dataFilePath = dirIt.path();
        std::size_t dataFileLen = stdfs::file_size(dataFilePath);


        stdfs::path summaryOutputPath = FormatSummaryOutputPath(summaryDirPath, summaryCount);

        std::promise<void> onExecuting{};
        std::future<void> onExecutingFtr = onExecuting.get_future();

        // start program
        std::thread commandThread{
            [=, &onExecuting] {
                std::string command = FormatCommand(
                    networkDeviceIP,
                    chunkerWindowWidth,
                    chunkerThreadnum,
                    chunkProcessThreadnum,
                    summaryOutputPath.generic_string(),
                    DBPassword
                );
                std::cout << command << '\n';

                onExecuting.set_value();
                system(command.c_str());
                std::cout << "\n\n";
            }
        };




        // generate summary hdr
        std::thread summaryHeaderThread{
            [=] {
                std::ofstream summaryOutputFile {summaryOutputPath, std::ios::out};
                summaryOutputFile << "{\n";





                // get FileID and print to summary head
                std::string dataFileName = dataFilePath.filename().generic_string();


                sql::Connection *  con = getConnection(DBPassword);


                sql::PreparedStatement *  getFileIDStmt = con->prepareStatement(
                    "SELECT file_id "
                    "FROM files "
                    "WHERE (file_name, file_length) = (?, ?)"
                );
                sql::ResultSet *  getFileIDRes = nullptr;
                getFileIDStmt->setString(1, dataFileName);
                getFileIDStmt->setUInt64(2, dataFileLen);
        
                getFileIDRes = getFileIDStmt->executeQuery();
                if (getFileIDRes->next()) {
                    summaryOutputFile 
                        << "\t\"Answer\" : " << getFileIDRes->getUInt("file_id") << ",\n";
                }

                delete getFileIDRes;
                delete getFileIDStmt;
                delete con;
                summaryOutputFile.close();
            }
        };





        // send dataFile
        // run socket on ipv4, stream-y, tcp
        SOCKET sendSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sendSocket < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }
        
        // init addr
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50000);
        serverAddr.sin_addr.s_addr = inet_addr(ServerIP.c_str());



        if (0 >
            connect(
                sendSocket, 
                (struct sockaddr*)&serverAddr,
                sizeof(serverAddr)
            )
        ) {
            perror("connect failed");
            CLOSE_SOCKET(sendSocket);
            std::exit(EXIT_FAILURE);
        }


        char* dataBuffer = new char[dataFileLen + 1];
        std::ifstream dataFileHandle {dataFilePath, std::ios::binary};
        if (!dataFileHandle.read(dataBuffer, dataFileLen)) {
            std::cerr << "Failed to read file!\n";
            continue;
        }

        onExecutingFtr.wait();
        std::this_thread::sleep_for(2s);
        if (send(sendSocket, dataBuffer, dataFileLen, 0) < 0) {
            perror("send failed");
            CLOSE_SOCKET(sendSocket);
            std::exit(EXIT_FAILURE);
        }





        // send Terminate Symbol
        std::this_thread::sleep_for(5s);
        summaryHeaderThread.join();
        stdfs::create_directories(terminateSymbol);
        commandThread.join();



        CLOSE_SOCKET(sendSocket);
        delete[] dataBuffer;
        summaryCount++;
    }






    return 0;
}