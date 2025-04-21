#include<filesystem>
namespace stdfs = std::filesystem;
#include<sstream>
#include<iostream>
#include<fstream>



// #include

std::string FormatCommand(
    std::string sourceFilePath,
    int chunkerWindowWidth,
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
        << "SourceType=File " 
        << "SourceFilePath=" << sourceFilePath << ' '
        << "ChunkerWindowWidth=" << chunkerWindowWidth << ' '
        << "ChunkerThreadnum=2 "
        << "ChunkProcessorActionMode=RecordToDataBase "
        << "ChunkProcessThreadnum=" << chunkProcessThreadnum << ' '
        << "SummaryOutputPath=" << summaryOutputPath << ' '
        << "DBPassword=" << DBPassword;
    return oss.str();
}


std::string FormatSummaryDirPath(int chunkerWindowWidth, int chunkProcessThreadnum) {
    std::ostringstream oss;
    oss << "./Summary/Record/w" << chunkerWindowWidth << "/j2." << chunkProcessThreadnum;
    return oss.str();
}



stdfs::path FormatSummaryOutputPath(stdfs::path summaryDirPath, int summaryCount) {
    std::ostringstream oss;
    oss << summaryCount << ".json";
    return summaryDirPath / oss.str();
}



/**
 * order of argc:
 * 
 * 1) chunkerWindowWidth
 * 2) chunkProcessThreadnum
 * 
 * 3) DBPassword
 */
int main(int argc, char * argv[]) {
    // check dataset
    stdfs::path datasetPath {"./Dataset"};
    if (!stdfs::is_directory(datasetPath)) {
        std::cout << "Dataset " << datasetPath << " doesn't exist!\n";
        std::exit(EXIT_FAILURE);
    }


    // get console args
    int chunkerWindowWidth = atoi(argv[1]);
    int chunkProcessThreadnum = atoi(argv[2]);
    std::string DBPassword {argv[3]};


    // FormatSummaryDirPath
    stdfs::path summaryDirPath = FormatSummaryDirPath(chunkerWindowWidth, chunkProcessThreadnum);
    if (!stdfs::is_directory(summaryDirPath)) {
        stdfs::create_directories(summaryDirPath);
    }





    // visit every file in dataset
    int summaryCount = 0;
    for (auto& dirIt : stdfs::directory_iterator{datasetPath}) {
        if (!dirIt.is_regular_file()) {
            continue;
        }


        // write summary head
        stdfs::path summaryOutputPath = FormatSummaryOutputPath(summaryDirPath, summaryCount);

        std::ofstream summaryOutputFile {summaryOutputPath, std::ios::out};
        summaryOutputFile << "{\n";
        summaryOutputFile.close();


        // execute program
        std::string command = FormatCommand(
            dirIt.path().generic_string(),
            chunkerWindowWidth,
            chunkProcessThreadnum,
            summaryOutputPath.generic_string(),
            DBPassword
        );
        std::cout << command << '\n';
        system(command.c_str());
        std::cout << "\n\n";

        summaryCount++;
    }

}