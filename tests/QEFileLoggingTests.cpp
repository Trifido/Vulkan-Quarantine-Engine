#include <Logging/QEFileLogSink.h>
#include <Logging/QELogger.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path testDirectory =
        std::filesystem::temp_directory_path() /
        ("qe-file-logging-test-" + std::to_string(uniqueValue));
    const std::filesystem::path logPath = testDirectory / "session.log";

    try
    {
        QEFileLogSink sink(logPath);
        QELogger::Get().AddSink(&sink);
        QELogger::Get().Warning("persistent message", "LoggingTest");
        QELogger::Get().RemoveSink(&sink);

        std::ifstream stream(logPath);
        const std::string contents(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());

        if (contents.find("[Warning] [LoggingTest]") == std::string::npos ||
            contents.find("persistent message") == std::string::npos ||
            contents.find("[Thread ") == std::string::npos)
        {
            std::cerr << "Persistent log did not contain the expected structured entry\n";
            return 1;
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "File logging test failed: " << exception.what() << '\n';
        return 1;
    }

    std::error_code error;
    std::filesystem::remove_all(testDirectory, error);
    return 0;
}
