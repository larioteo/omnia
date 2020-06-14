#pragma once
#include "Omnia.pch"

#include "Types.h"
#include "System/Cli.h"
#include "Utility/DateTime.h"

#pragma warning(push)
#pragma warning(disable: 26812)

namespace Omnia {

using std::is_same_v;
using std::mutex;
using std::ofstream;
using std::ostream;

enum class LogLevel {
	Trace,
	Verbose,
	Debug,
	Information,
	Warning,
	Error,
	Critical,
	Default,
};

enum class LogType {
	Default,
	Caption,
	Process,
	Success,
	Warning,
	Failure,
};

class Log {
	// Properties
	static inline uint64_t Counter = 0;
	static inline string LogFile = "./Log/Ultra.log"s;
	static inline mutex Sync;
	static inline mutex SyncFile;
	static inline ofstream FileStream;
	ostream &os = std::cout;

	// Constructors and Deconstructors
	Log() = default;
	~Log() { Finish(); }
	Log(const Log &) {}
	Log(Log &&) noexcept {}

	// Operators
	Log &operator=(const Log &) {}
	Log &operator=(const Log &&) noexcept {}

public:
	// Enumerations
	enum FType {
		Header,
		Section,
		Delimiter,
		Footer,
	};
	enum Type {
		Default,
		Area,
		Info,
		Note,
		Error,
		Critical,
		Debug,
		Verbose,

		Caption,
		Process,
		Success,
		Warning,
		Failure,
	};

	// Methods
	static Log &Instance() {
		static Log instance;
		return instance;
	}

	template <typename T>
	Log &operator<<(const T &data) {
		std::unique_lock<mutex> lock(Sync);
		std::unique_ptr<stringstream> cli{new stringstream};
		std::unique_ptr<stringstream> file{new stringstream};

		if (!FileStream.is_open()) { Start(); }

		// Concat timestamp
		if constexpr (std::is_same_v<T, Type>) {
			if (!(data == Area || data == Caption || data == Default)) {
				os << Cli::Color::DarkGrey << apptime.GetTimeStamp("%Y-%m-%dT%H:%M") << " | "; 
				*file << apptime.GetTimeStamp() << " | ";
			}
		}

		// Write Log-File specific stuff
		if constexpr (std::is_same_v<T, FType>) {
			switch (data) {
				case Header:    WriteHeader();    break;
				case Section:   WriteSection();   break;
				case Delimiter: WriteDelimiter(); break;
				case Footer:    WriteFooter();    break;
			}
			return (*this);
		}

		// Set color and symbol
		if constexpr (std::is_same_v<T, Type>) {
			switch (data) {
				case Area:      os << Cli::Color::LightBlue << "\n";    *file << "\n";  break;
				case Info:      os << Cli::Color::LightGrey << "✶ ";    *file << "✶ ";  break;
				case Note:      os << Cli::Color::Green << "≡ ";        *file << "≡ ";  break;
				case Error:     os << Cli::Color::Red << "˃ ";          *file << "˃ ";  break;
				case Debug:     os << Cli::Color::Yellow << "■ ";       *file << "■ ";  break;
				case Verbose:   os << Cli::Color::LightMagenta << "◙ "; *file << "◙ ";  break;

				case Caption:   os << Cli::Color::LightBlue << "\n";    *file << "\n";  break;
				case Process:   os << Cli::Color::Cyan << "҉ ";         *file << "҉ ";   break;
				case Success:   os << Cli::Color::LightCyan << "○ ";    *file << "○ ";  break;
				case Warning:   os << Cli::Color::LightYellow << "□ ";  *file << "□ ";  break;
				case Failure:   os << Cli::Color::LightRed << "♦ ";     *file << "♦ ";  break; 
				default:        os << Cli::Color::White << "";          *file << "";    break;
			}
			Write(file->str());
			++Counter;
			return (*this);
		}

		// Automatically reset to default color after newline
		if constexpr (std::is_same_v<T, char[2]>) {
			if (data == "\n"s || data == "\r"s || data == "\r\n"s) {
				*cli << Cli::Style::Reset;
				*cli << Cli::Color::White << "";
			}
		}

		// Stream data
		*cli << data;
		*file << data;
		os << cli->str();
		Write(file->str());
		++Counter;
		return (*this);
	}
	Log &operator<<(std::ostream &(*T)(std::ostream &)) {
		os << T;
		return (*this);
	}

	static void Test() {
		Log::Instance() << Log::Area << "Area" << "\n";
		Log::Instance() << Log::Info << "Info" << "\n";
		Log::Instance() << Log::Note << "Note" << "\n";
		Log::Instance() << Log::Error << "Error" << "\n";
		Log::Instance() << Log::Debug << "Debug" << "\n";
		Log::Instance() << Log::Verbose << "Verbose" << "\n";

		Log::Instance() << Log::Caption << "Caption" << "\n";
		Log::Instance() << Log::Process << "Process" << "\n";
		Log::Instance() << Log::Success << "Success" << "\n";
		Log::Instance() << Log::Warning << "Warning" << "\n";
		Log::Instance() << Log::Failure << "Failure" << "\n";
	}

private:
	// Log-File handling
	static void Finish() { FileStream.close(); }
	static void Start(const std::filesystem::path &object = LogFile) {
		auto Directory = object.parent_path();
		if (!Directory.empty()) std::filesystem::create_directories(Directory);
		FileStream.open(object);
	}
	static void Write(const string &message) {
		std::unique_lock<mutex> lock(SyncFile);
		if (FileStream.is_open()) {
			FileStream << message;
		}
	}

	// Log-File specific Features
	static void WriteHeader() {
		Write("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
		Write("Ultra\n");
		Write("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
	}
	static void WriteSection() {
		Write("▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬\n");
	}
	static void WriteDelimiter() {
		Write("----------------------------------------------------------------\n");
	}
	static void WriteFooter() {
		Write("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
	}
};

inline Log &applog = Log::Instance();
inline Log &appout = Log::Instance();

#ifdef _DEBUG
	#define APP_LOG(x)			applog << Log::Default	<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_INFO(x)		applog << Log::Info		<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_WARN(x)		applog << Log::Warning	<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_ERROR(x)	applog << Log::Error	<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_FATAL(x)	applog << Log::Failure	<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_DEBUG(x)	applog << Log::Debug	<< "{" __FUNCTION__ << "}: " << x << "\n"
	#define APP_LOG_TRACE(x)	applog << Log::Verbose	<< "{" __FUNCTION__ << "}: " << x << "\n"

	//#define APP_ASSERT(x, ...) { if(!(x)) { APP_LOG_ERROR("Assertation Failed: ", __VA_ARGS__); APP_DEBUGBREAK(); } }
#else
	#define APP_LOG(x)
	#define APP_LOG_INFO(x)
	#define APP_LOG_WARN(x)
	#define APP_LOG_ERROR(x)
	#define APP_LOG_FATAL(x)
	#define APP_LOG_DEBUG(x)
	#define APP_LOG_TRACE(x)
#endif

}

#pragma warning(pop)
