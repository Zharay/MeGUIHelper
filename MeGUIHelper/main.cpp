#ifdef MEDIAINFO_LIBRARY
    #include "MediaInfo/MediaInfo.h" //Staticly-loaded library (.lib or .a or .so)
    #define MediaInfoNameSpace MediaInfoLib;
#else //MEDIAINFO_LIBRARY
    #include "MediaInfoDLL/MediaInfoDLL.h" //Dynamicly-loaded library (.dll or .so)
    #define MediaInfoNameSpace MediaInfoDLL;
#endif //MEDIAINFO_LIBRARY
using namespace MediaInfoNameSpace;

#ifdef __MINGW32__
    #ifdef _UNICODE
        #define _itot _itow
    #else //_UNICODE
        #define _itot itoa
    #endif //_UNICODE
#endif //__MINGW32

#include <iostream>
#include <fstream>
#include <process.h>
#include <direct.h>
#include <vector>
#include <string>
#include <iterator>
#include <direct.h>
#include <regex>
#include <Windows.h>
#include <map>
#include <locale>
#include <codecvt>
#include <TlHelp32.h>
#include <filesystem>

#include <boost/integer/common_factor_rt.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
namespace po = boost::program_options;

#pragma warning(disable : 4996)

using namespace std;
using namespace boost;

/******************************************************************/
//	Structures and Enums
/******************************************************************/
struct trackInfo {
	String filename = L"";
	String extension = L"";
	String language = L"";
	String framerate = L"23.976";
	String AR = L"1.7777777777777777777777777778";
	String ARx = L"16";
	String ARy = L"9";
	int trackNum = -1;
	bool bColorCorrect = false;
	bool bReencode = false;
	bool bIsFont = false;
	bool bIsExternal = false;
};

struct videoFile {
	String filePath = L"";
	String fileName = L"";
	String parentDir = L"";
	String parentDirName = L"";
	String outFileName = L"";
	String subDir = L"";

	trackInfo videoTrack;
	vector<trackInfo> audioTracks;
	vector<trackInfo> subtitleTracks;
	vector<trackInfo> attachmentTracks;
	int selectedAudioTrack = 0;
	int selecteSubtitleTrack = -1;
	int jobNum = -1;
};

enum cmd_color {
	msg_norm = 7,
	msg_info = 10,
	msg_erro = 12,
	msg_warn = 14,
	msg_whit = 15
};

map<String, String> Languages;
vector<videoFile> videoList;
map<String, bool> attchmentList;
int maxJobs = 0;
int jobNumber = 1;
bool bExtractedAttachments = false;

/******************************************************************/
//	Program Parameters
/******************************************************************/
String MeGUIDir = L"";
String WorkDir = L"";
String OutputDir = L"";
String defaultAudioLang = L"jpn";
String defaultSubtitleLang = L"eng";
String customEpisodeRegex = L"";
int forcedAudioTrack = -1;
int forcedSubtitleTrack = -1;
bool bCleanFilename;
bool bCreateSubDir;
bool bRelativeTrack = true;
bool bDoAttachments = true;
bool bInstallFonts = true;
bool bAggressiveClean;
bool bJobFilesOnly;
bool bCleanParenthesis;
bool bClearMeGUIJobs;
bool bExtract264;
bool bChooseTrack;
bool bUseConditionalExternalSubs = true;
bool bVerbose;
bool bEnableSUP;

/******************************************************************/
//	Raw MeGUI Job Information
/******************************************************************/
//const String AVSTemplate = L"LoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\lsmash\\LSMASHSource.dll\")\nLWLibavVideoSource(\"%1%\"%2%)\n";
//const String AVSTemplate_264 = L"LoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\ffms\\ffms2.dll\")\nFFVideoSource(\"%2%%1%.264\"%3%)\n";
//const String AVSTemplate_Subs = L"\nLoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\avisynth_plugin\\VSFilter.dll\")\nTextSub(\"%2%%1%.%3%\", 1)";
//const String AVSTemplate_Sups = L"\nLoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\avisynth_plugin\\SupTitle.dll\")\nTextSub(\"%2%%1%.%3%\")";

const String AVSTemplate = L"LoadPlugin(\"%1%\\tools\\lsmash\\LSMASHSource.dll\")\nLWLibavVideoSource(\"%2%\"%3%)\n";
const String AVSTemplate_264 = L"LoadPlugin(\"%1%\\tools\\ffms\\ffms2.dll\")\nFFVideoSource(\"%3%%2%.264\"%4%)\n";
const String AVSTemplate_Subs = L"\nLoadPlugin(\"%1%\\tools\\avisynth_plugin\\VSFilter.dll\")\nTextSub(\"%3%%2%.%4%\", 1)";
const String AVSTemplate_Sups = L"\nLoadPlugin(\"%1%\\tools\\avisynth_plugin\\SupTitle.dll\")\nTextSub(\"%3%%2%.%4%\")";
const String videoJobTemplate = L"<?xml version=\"1.0\" encoding=\"windows-1252\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n\t<EncodingSpeed />\n\t<Job xsi:type=\"VideoJob\">\n\t<Input>%2%%1%.avs</Input>\n\t<Output>%3%%1%-1.264</Output>\n\t<FilesToDelete />\n\t<Zones />\n\t<DAR>\n\t\t<AR>%4%</AR>\n\t\t<X>%5%</X>\n\t\t<Y>%6%</Y>\n\t</DAR>\n\t<Settings xsi:type=\"x264Settings\">\n\t\t<VideoEncodingType>quality</VideoEncodingType>\n\t\t<BitrateQuantizer>20</BitrateQuantizer>\n\t\t<KeyframeInterval>250</KeyframeInterval>\n\t\t<NbBframes>3</NbBframes>\n\t\t<MinQuantizer>0</MinQuantizer>\n\t\t<MaxQuantizer>81</MaxQuantizer>\n\t\t<V4MV>false</V4MV>\n\t\t<QPel>false</QPel>\n\t\t<Trellis>false</Trellis>\n\t\t<CreditsQuantizer>40</CreditsQuantizer>\n\t\t<Logfile>%3%%1%.stats</Logfile>\n\t\t<VideoName />\n\t\t<CustomEncoderOptions />\n\t\t<MaxNumberOfPasses>3</MaxNumberOfPasses>\n\t\t<NbThreads>0</NbThreads>\n\t\t<x264PresetLevel>medium</x264PresetLevel>\n\t\t<x264PsyTuning>NONE</x264PsyTuning>\n\t\t<QuantizerCRF>20.0</QuantizerCRF>\n\t\t<InterlacedMode>progressive</InterlacedMode>\n\t\t<TargetDeviceXML>12</TargetDeviceXML>\n\t\t<BlurayCompatXML>False</BlurayCompatXML>\n\t\t<NoDCTDecimate>false</NoDCTDecimate>\n\t\t<PSNRCalculation>false</PSNRCalculation>\n\t\t<NoFastPSkip>false</NoFastPSkip>\n\t\t<NoiseReduction>0</NoiseReduction>\n\t\t<NoMixedRefs>false</NoMixedRefs>\n\t\t<X264Trellis>1</X264Trellis>\n\t\t<NbRefFrames>3</NbRefFrames>\n\t\t<AlphaDeblock>0</AlphaDeblock>\n\t\t<BetaDeblock>0</BetaDeblock>\n\t\t<SubPelRefinement>7</SubPelRefinement>\n\t\t<MaxQuantDelta>4</MaxQuantDelta>\n\t\t<TempQuantBlur>0</TempQuantBlur>\n\t\t<BframePredictionMode>1</BframePredictionMode>\n\t\t<VBVBufferSize>31250</VBVBufferSize>\n\t\t<VBVMaxBitrate>31250</VBVMaxBitrate>\n\t\t<METype>1</METype>\n\t\t<MERange>16</MERange>\n\t\t<MinGOPSize>25</MinGOPSize>\n\t\t<IPFactor>1.4</IPFactor>\n\t\t<PBFactor>1.3</PBFactor>\n\t\t<ChromaQPOffset>0</ChromaQPOffset>\n\t\t<VBVInitialBuffer>0.9</VBVInitialBuffer>\n\t\t<BitrateVariance>1.0</BitrateVariance>\n\t\t<QuantCompression>0.6</QuantCompression>\n\t\t<TempComplexityBlur>20</TempComplexityBlur>\n\t\t<TempQuanBlurCC>0.5</TempQuanBlurCC>\n\t\t<SCDSensitivity>40</SCDSensitivity>\n\t\t<BframeBias>0</BframeBias>\n\t\t<PsyRDO>1.0</PsyRDO>\n\t\t<PsyTrellis>0</PsyTrellis>\n\t\t<Deblock>true</Deblock>\n\t\t<Cabac>true</Cabac>\n\t\t<UseQPFile>false</UseQPFile>\n\t\t<WeightedBPrediction>true</WeightedBPrediction>\n\t\t<WeightedPPrediction>2</WeightedPPrediction>\n\t\t<NewAdaptiveBFrames>1</NewAdaptiveBFrames>\n\t\t<x264BFramePyramid>2</x264BFramePyramid>\n\t\t<x264GOPCalculation>1</x264GOPCalculation>\n\t\t<ChromaME>true</ChromaME>\n\t\t<MacroBlockOptions>3</MacroBlockOptions>\n\t\t<P8x8mv>true</P8x8mv>\n\t\t<B8x8mv>true</B8x8mv>\n\t\t<I4x4mv>true</I4x4mv>\n\t\t<I8x8mv>true</I8x8mv>\n\t\t<P4x4mv>false</P4x4mv>\n\t\t<AdaptiveDCT>true</AdaptiveDCT>\n\t\t<SSIMCalculation>false</SSIMCalculation>\n\t\t<StitchAble>false</StitchAble>\n\t\t<QuantizerMatrix>Flat (none)</QuantizerMatrix>\n\t\t<QuantizerMatrixType>0</QuantizerMatrixType>\n\t\t<DeadZoneInter>21</DeadZoneInter>\n\t\t<DeadZoneIntra>11</DeadZoneIntra>\n\t\t<X26410Bits>false</X26410Bits>\n\t\t<OpenGop>False</OpenGop>\n\t\t<X264PullDown>0</X264PullDown>\n\t\t<SampleAR>0</SampleAR>\n\t\t<ColorMatrix>0</ColorMatrix>\n\t\t<ColorPrim>0</ColorPrim>\n\t\t<Transfer>0</Transfer>\n\t\t<AQmode>1</AQmode>\n\t\t<AQstrength>1.0</AQstrength>\n\t\t<QPFile />\n\t\t<Range>auto</Range>\n\t\t<x264AdvancedSettings>true</x264AdvancedSettings>\n\t\t<Lookahead>40</Lookahead>\n\t\t<NoMBTree>true</NoMBTree>\n\t\t<ThreadInput>true</ThreadInput>\n\t\t<NoPsy>false</NoPsy>\n\t\t<Scenecut>true</Scenecut>\n\t\t<Nalhrd>0</Nalhrd>\n\t\t<X264Aud>false</X264Aud>\n\t\t<X264SlowFirstpass>false</X264SlowFirstpass>\n\t\t<PicStruct>false</PicStruct>\n\t\t<FakeInterlaced>false</FakeInterlaced>\n\t\t<NonDeterministic>false</NonDeterministic>\n\t\t<SlicesNb>0</SlicesNb>\n\t\t<MaxSliceSyzeBytes>0</MaxSliceSyzeBytes>\n\t\t<MaxSliceSyzeMBs>0</MaxSliceSyzeMBs>\n\t\t<Profile>2</Profile>\n\t\t<AVCLevel>L_41</AVCLevel>\n\t\t<TuneFastDecode>false</TuneFastDecode>\n\t\t<TuneZeroLatency>false</TuneZeroLatency>\n\t</Settings>\n\t</Job>\n\t<RequiredJobNames />\n\t<EnabledJobNames>\n\t<string>%1% - MKV</string>\n\t</EnabledJobNames>\n\t<Name>%1% - 264</Name>\n\t<Status>WAITING</Status>\n\t<Start>0001-01-01T00:00:00</Start>\n\t<End>0001-01-01T00:00:00</End>\n</TaggedJob>";
const String audioJobTemplate = L"<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"AudioJob\">\n    <Input>%2%%1%.%3%</Input>\n    <Output>%2%%1%.ac3</Output>\n    <FilesToDelete />\n    <CutFile />\n    <Settings xsi:type=\"AC3Settings\">\n      <PreferredDecoderString>LWLibavAudioSource</PreferredDecoderString>\n      <DownmixMode>KeepOriginal</DownmixMode>\n      <BitrateMode>CBR</BitrateMode>\n      <Bitrate>384</Bitrate>\n      <AutoGain>false</AutoGain>\n      <SampleRateType>deprecated</SampleRateType>\n      <SampleRate>KeepOriginal</SampleRate>\n      <TimeModification>KeepOriginal</TimeModification>\n      <ApplyDRC>false</ApplyDRC>\n      <Normalize>100</Normalize>\n      <CustomEncoderOptions />\n    </Settings>\n    <Delay>0</Delay>\n    <SizeBytes>0</SizeBytes>\n    <BitrateMode>CBR</BitrateMode>\n  </Job>\n  <RequiredJobNames />\n  <EnabledJobNames />\n  <Name>%1% - AC3</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>";
const String muxJobTemplate = L"<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"MuxJob\">\n    <Input>%3%%1%-1.264</Input>\n    <Output>%4%%1%.mkv</Output>\n    <FilesToDelete>\n      <string>%2%%1%.264</string>\n      <string>%9%.lwi</string>\n      <string>%3%%1%-1.264</string>\n      <string>%2%%1%.avs</string>\n      <string>%3%%1%.%5%</string>\n    </FilesToDelete>\n    <ContainerTypeString>MKV</ContainerTypeString>\n    <Codec />\n    <NbOfBFrames>0</NbOfBFrames>\n    <NbOfFrames>0</NbOfFrames>\n    <Bitrate>0</Bitrate>\n    <Overhead>4.3</Overhead>\n    <Settings>\n      <MuxedInput />\n      <MuxedOutput>%4%%1%.mkv</MuxedOutput>\n      <VideoInput>%3%%1%-1.264</VideoInput>\n      <AudioStreams>\n        <MuxStream>\n          <path>%3%%1%.%5%</path>\n          <delay>0</delay>\n          <bDefaultTrack>false</bDefaultTrack>\n          <bForceTrack>false</bForceTrack>\n          <language>Japanese</language>\n          <name />\n        </MuxStream>\n      </AudioStreams>\n      <SubtitleStreams />\n      <Framerate%6%\n      <ChapterInfo>\n        <Title />\n        <SourceFilePath />\n        <SourceType />\n        <FramesPerSecond>0</FramesPerSecond>\n        <TitleNumber>0</TitleNumber>\n        <PGCNumber>0</PGCNumber>\n        <AngleNumber>0</AngleNumber>\n        <Chapters />\n        <DurationTicks>0</DurationTicks>\n      </ChapterInfo>\n      <Attachments />\n      <SplitSize xsi:nil=\"true\" />\n      <DAR xsi:nil=\"true\" />\n      <DeviceType>Standard</DeviceType>\n      <VideoName>%1%</VideoName>\n      <MuxAll>false</MuxAll>\n    </Settings>\n    <MuxType>MKVMERGE</MuxType>\n  </Job>\n  <RequiredJobNames>\n  <string>%1% - 264</string>\n  </RequiredJobNames>\n  <EnabledJobNames />\n  <Name>%1% - MKV</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>";
const String muxJobTemplate_264 = L"<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"MuxJob\">\n    <Input>%3%%1%-1.264</Input>\n    <Output>%4%%1%.mkv</Output>\n    <FilesToDelete>\n      <string>%2%%1%.264</string>\n      <string>%2%%1%.264.ffindex</string>\n      <string>%3%%1%-1.264</string>\n      <string>%2%%1%.avs</string>\n      <string>%3%%1%.%5%</string>\n    </FilesToDelete>\n    <ContainerTypeString>MKV</ContainerTypeString>\n    <Codec />\n    <NbOfBFrames>0</NbOfBFrames>\n    <NbOfFrames>0</NbOfFrames>\n    <Bitrate>0</Bitrate>\n    <Overhead>4.3</Overhead>\n    <Settings>\n      <MuxedInput />\n      <MuxedOutput>%4%%1%.mkv</MuxedOutput>\n      <VideoInput>%3%%1%-1.264</VideoInput>\n      <AudioStreams>\n        <MuxStream>\n          <path>%3%%1%.%5%</path>\n          <delay>0</delay>\n          <bDefaultTrack>false</bDefaultTrack>\n          <bForceTrack>false</bForceTrack>\n          <language>Japanese</language>\n          <name />\n        </MuxStream>\n      </AudioStreams>\n      <SubtitleStreams />\n      <Framerate%6%\n      <ChapterInfo>\n        <Title />\n        <SourceFilePath />\n        <SourceType />\n        <FramesPerSecond>0</FramesPerSecond>\n        <TitleNumber>0</TitleNumber>\n        <PGCNumber>0</PGCNumber>\n        <AngleNumber>0</AngleNumber>\n        <Chapters />\n        <DurationTicks>0</DurationTicks>\n      </ChapterInfo>\n      <Attachments />\n      <SplitSize xsi:nil=\"true\" />\n      <DAR xsi:nil=\"true\" />\n      <DeviceType>Standard</DeviceType>\n      <VideoName>%1%</VideoName>\n      <MuxAll>false</MuxAll>\n    </Settings>\n    <MuxType>MKVMERGE</MuxType>\n  </Job>\n  <RequiredJobNames>\n  <string>%1% - 264</string>\n  </RequiredJobNames>\n  <EnabledJobNames />\n  <Name>%1% - MKV</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>";
const String jobListTemplate = L"<?xml version=\"1.0\"?>\n<JobListSerializer xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n\t<mainJobList>%1%</mainJobList>\n\t<workersAndTheirJobLists>\n\t\t<PairOfStringListOfString>\n\t\t\t<fst>Worker 1</fst>\n\t\t\t<snd />\n\t\t</PairOfStringListOfString>\n\t</workersAndTheirJobLists>\n</JobListSerializer>";

/******************************************************************/
//	Function Declarations
/******************************************************************/

// Initialize Functions
int processOptions(int ac, wchar_t* av[]);
void initLang();

// System Utility Functions
void MsgColor(String msg, WORD color);
void MsgColor(string msg, WORD color);
DWORD FindProcessId(const String & processName);
bool TerminateProcess(DWORD processID);
bool DoesFileExist(String filename);

// Video Info Functiuons
videoFile getVideoInfo(const wchar_t* filepath, int jobNum);
void cleanFilename(videoFile &fileInfo);
void selectTracks(videoFile &fileInfo);

// Video Extraction
void extractMKV(videoFile fileInfo);
void extractMP4(videoFile fileInfo);

// MeGUI Jobs
void clearMeGUIJobs();
void createMeGUIJobs(videoFile fileInfo);

/******************************************************************/
//	Main Function
/******************************************************************/

/**
	Can take in a list of dropped files as its inputs by default.
	@TODO : Handling dropped folders
 **/
int wmain(int argc, wchar_t *argv[])
{
	videoFile tempVideo;
	String tempArg;
	vector<String> optionArgs;
	vector<String> vidPaths;

	// First, find out if MeGUI is running and ask to close it.
	// If you don't, the program will rewrite all the job list to not include anything we add.
	if (FindProcessId(L"MeGUI.exe"))
	{
		MsgColor(L"Please close MeGUI first!\n", msg_erro);
		wcout << "Close MeGUI now? [Y] : ";
		getline(wcin, tempArg);
		if (tempArg != L"" && tempArg != L"y" && tempArg != L"Y")
			return errno;

		TerminateProcess(FindProcessId(L"MeGUI.exe"));

		Sleep(100);

		if (FindProcessId(L"MeGUI.exe"))
		{
			MsgColor(L"Could not close MeGUI?", msg_erro);
			getline(wcin, tempArg);
			return errno;
		}
	}

	// Initial parsing of the arguments, culling out anything without the proper formats.
	for (size_t i = 0; i < argc; ++i)
	{
		tempArg = argv[i];		// Easier to handle strings than character strings.
		if (tempArg.length() > 4 && (tempArg.substr(tempArg.length() - 4, 4) == L".mkv" || tempArg.substr(tempArg.length() - 4, 4) == L".mp4"))
		{ // Video files
			vidPaths.push_back(tempArg);
			maxJobs++;
		}
		else // Options for Boost to parse.
			optionArgs.push_back(tempArg);
	}

	// Unfortunately, processOptions, which uses Boost, doesn't like anything other than a raw character string.
	//	This is to fix that.
	wchar_t ** av = new wchar_t*[optionArgs.size()];
	for (size_t i = 0; i < optionArgs.size(); i++)
	{
		av[i] = new wchar_t[optionArgs[i].size() + 1];
		wcscpy(av[i], optionArgs[i].c_str());
	}

	if (processOptions(optionArgs.size(), av))
	{
		MsgColor(L"Error loading options/config.", msg_erro);
		getline(wcin, tempArg);
		return errno;
	}

	for (size_t i = 0; i < optionArgs.size(); i++)
	{
		delete [] av[i];
	}
	delete[] av;
	// Finished processing options

	// Initialize the language map
	initLang();

	// Beginning media file parsing.
	for (int i = 0; i < vidPaths.size(); i++)
	{
		tempVideo = getVideoInfo(vidPaths[i].data(), i + 1);
		
		if (tempVideo.fileName == L"")
		{ // If blank, ignore the file.
			continue;
		}
		else if (tempVideo.audioTracks.empty())
		{ // If no audio, ignore the file (Possible error with file?) (Should I handle it still?)
			MsgColor(L"ERROR: No audio detected! Skipping", msg_erro);
			continue;
		}

		videoList.push_back(tempVideo);
	}

	// We have videos! Now to clean them and process their jobs.
	if (videoList.size() != 0)
	{
		if (bClearMeGUIJobs)
			clearMeGUIJobs();
		for (int i = 0; i <= (int(videoList.size()) - 1); i++)
		{
			if (!bJobFilesOnly && videoList[i].videoTrack.extension == L"mkv")
				extractMKV(videoList[i]);
			else if (!bJobFilesOnly)
				extractMP4(videoList[i]);
			createMeGUIJobs(videoList[i]);
		}

		// @TODO : Installing fonts from command line! I could not figure it out without some complex code.
		if (bInstallFonts && !bJobFilesOnly && bExtractedAttachments)
		{ // For now it just opents the folder containing any fonts found. You have to manually install them yourself.
			wcout << "Open font folder and run MeGUI? (Y) ";
			getline(wcin, tempArg);
			if (tempArg == L"y" || tempArg == L"Y" || tempArg == L"")
			{
				_wsystem(String(L"start " + MeGUIDir + L"MeGUI.exe").data());
				_wsystem(String(L"start " + WorkDir + L"Fonts").data());
			}
		}
		else
		{
			wcout << "Run MeGUI? (Y) ";
			getline(wcin, tempArg);
			if (tempArg == L"y" || tempArg == L"Y" || tempArg == L"")
				_wsystem(String(L"start " + MeGUIDir + L"MeGUI.exe").data());
		}

	}
	else
	{
		wcout << ": No MKV files in arguement!" << endl << "Use: MeGUIHelper.exe [options] [filename1] [filename2] ..." << endl;
		getline(wcin, tempArg);
	}

	return 0;
}

/******************************************************************/
//	Init Functions
/******************************************************************/

// Using Boost's options library to handle flag options and config file.
int processOptions(int ac, wchar_t* av[])
{
	try {
		String config_file;

		string _config_file, _MeGUIDir, _WorkDir, _OutputDir, _defaultAudioLang, _defaultSubtitleLang, _customEpisodeRegex;

		po::options_description basic("Base Options");
		basic.add_options()
			("help,h", "Produce help message")
			("config,c", po::value<string>(&_config_file)->default_value("MeGUIHelper.cfg"), "Name of configuration file.")
			("bVerbose", po::value<bool>(&bVerbose)->default_value(false), "Show all media information.")
			;

		po::options_description directories("Directories");
		directories.add_options()
			("MeGUI-Dir", po::value< string >(&_MeGUIDir), "Path to MeGUI (Required)")
			("Work-Dir,w", po::value< string >(&_WorkDir), "Temporary raw file location\n(Default: MeGUI Helper's location)")
			("Output-Dir,o", po::value< string >(&_OutputDir), "Output of encoded files\n(Default: MeGUI Helper's location)")
			;

		po::options_description operations("Operation Options");
		operations.add_options()
			("bUseConditionalExternalSubs,x", po::value<bool>(&bUseConditionalExternalSubs)->default_value(true), "If no sub was found in the file, it will locate an external sub file under the same name.")
			("bJobFilesOnly,J", po::value<bool>(&bJobFilesOnly)->default_value(false), "Create job scripts only. (DOES NOT EXTRACT)")
			("bClearMeGUIJobs,j", po::value<bool>(&bClearMeGUIJobs)->default_value(false), "Clean all old jobs (MeGUI must be closed!).")
			("bEnableSUP,U", po::value<bool>(&bEnableSUP)->default_value(false), "Enable SUP subtitle support (Requires SupTitle and SupCore in AviSynth's addons!).")
			;

		po::options_description languages("Language Options");
		languages.add_options()
			("AudioLang,A", po::value<string>(&_defaultAudioLang)->default_value("jpn"), "Default audio language")
			("SubtitleLang,S", po::value<string>(&_defaultSubtitleLang)->default_value("eng"), "Default subtitle language")
			("AudioTrack,a", po::value<int>(&forcedAudioTrack)->default_value(-1), "Force all videos to use this audio track")
			("SubtitleTrack,s", po::value<int>(&forcedSubtitleTrack)->default_value(-1), "Force all videos to use this subtitle track")
			("bRelativeTrack,r", po::value<bool>(&bRelativeTrack)->default_value(true), "Make forced tracks relative \n(Example: '-s 1' will select the first track of two available)")
			("bChooseTrack,M", po::value<bool>(&bChooseTrack)->default_value(false), "Manually set suitable track if one cannot be found.")
			;

		po::options_description output("Output Options");
		output.add_options()
			("bExtractVideo,V", po::value<bool>(&bExtract264)->default_value(false), "Extract video track. CAUTION: Has a tendency to break without proper timecodes.")
			("bCleanFilename,C", po::value<bool>(&bCleanFilename)->default_value(true), "Clean up beginning [group] tags and fix spaces")
			("bAggressiveClean,a", po::value< bool >(&bAggressiveClean), "Aggressively remove any and all junk [tags]")
			("bCleanParenthesis,p", po::value<bool>(&bCleanParenthesis)->default_value(false), "Remove anything within (parenthesis) (Requires aggressive file cleaning)")
			("bCreateSubDir,d", po::value<bool>(&bCreateSubDir)->default_value(true), "Create subdirectories using formatted filename")
			;

		po::options_description attachments("Attachment Options");
		attachments.add_options()
			("bDoAttachments,f", po::value<bool>(&bDoAttachments)->default_value(true), "Extract attachment files.")
			("bInstallFonts,F", po::value<bool>(&bDoAttachments)->default_value(true), "Automatically install font files. (Requires bDoAttachments)")
			;

		// Hidden options, will be allowed both on command line and in config file, but will not be shown to the user.
		po::options_description hidden("Hidden Options");
		hidden.add_options()
			("customEpisodeRegex", po::value<string>(&_customEpisodeRegex), "A custom regex string for episode counters")
			;

		po::options_description cmdline_options;
		cmdline_options.add(basic).add(directories).add(operations).add(languages).add(output).add(attachments).add(hidden);

		po::options_description config_file_options;
		config_file_options.add(directories).add(operations).add(languages).add(output).add(attachments).add(hidden);

		po::options_description visible("Allowed options");
		visible.add(basic).add(directories).add(operations).add(languages).add(output).add(attachments);

		po::positional_options_description p;
		p.add("input-file", -1);

		po::variables_map vm;
		store(po::wcommand_line_parser(ac, av).options(cmdline_options).positional(p).run(), vm);
		notify(vm);

		wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
		config_file = converter.from_bytes(_config_file);

		try
		{
			ifstream ifs(_config_file.c_str());
			if (!ifs)
			{
				MsgColor(L"ERROR: Cannot open config file: " + config_file, msg_erro);
				return errno;
			}
			else
			{
				store(parse_config_file(ifs, config_file_options), vm);
				notify(vm);
			}
		}
		catch (...)
		{
			MsgColor(L"ERROR: Something went wrong with the configuration! Most likely an unknown variable in the config file?", msg_erro);
			return errno;
		}

		MeGUIDir = converter.from_bytes(_MeGUIDir);
		WorkDir = converter.from_bytes(_WorkDir);
		OutputDir = converter.from_bytes(_OutputDir);
		defaultAudioLang = converter.from_bytes(_defaultAudioLang);
		defaultSubtitleLang = converter.from_bytes(_defaultSubtitleLang);
		customEpisodeRegex = converter.from_bytes(_customEpisodeRegex);

		// If all is well, config and options loaded and ready!
		if (vm.count("help")) {
			cout << visible << "\n";
			return 0;
		}

		if (vm.count("version")) {
			cout << "MeGUI Helper, version 1.0\n";
			return 0;
		}

		wchar_t * cPath;

		if (!(cPath = _wgetcwd(NULL, 0)))
			return errno;

		if (MeGUIDir.empty())
		{
			MsgColor(L"ERROR: Please fix your config file to include a working path to mkvtoolnix and MeGUI!", msg_erro);
			return errno;
		}
		else if (MeGUIDir[int(MeGUIDir.size()) - 1] != '//')
			MeGUIDir += L"\\";

		if (!WorkDir.empty())
		{
			if (WorkDir[int(WorkDir.size()) - 1] != '//')
				WorkDir += L"\\";
		}
		else
			WorkDir = String(cPath) + L"//";

		if (!OutputDir.empty())
		{
			if (OutputDir[int(OutputDir.size()) - 1] != '//')
				OutputDir += L"\\";
		}
		else
			OutputDir = String(cPath) + L"//";

		// Lastly, lets figure out if we have the proper DLLs.
		if (bEnableSUP && !DoesFileExist(MeGUIDir + L"\\tools\\avisynth_plugin\\SupTitle.dll"))
			MsgColor(L"WARNING: SUP subtitles requires SupTitle plugin in MeGUI's AviSynth plugin folder!", msg_erro);

		// A cool thing about using MeGUI, it already has MediaInfo.dll, so we copy that to use ourselves.
		if (!DoesFileExist(L"MediaInfo.dll") && DoesFileExist(MeGUIDir + L"\\MediaInfo.dll"))
		{
			try {
				MsgColor(L"Copying MediaInfo.dll from MeGUI", msg_info);
				CopyFile(const_cast<LPWSTR>(String(MeGUIDir + L"\\MediaInfo.dll").c_str()), L"MediaInfo.dll", true);
			}
			catch (std::exception e)
			{
				MsgColor("Copying MediaInfo.dll ERROR: " + string(e.what()), msg_erro);
			}
		}

	}
	catch (std::exception e)
	{
		wcout << e.what() << "\n";
		return errno;
	}
	return 0;
}

// Creates the map of known languages that is specific to MediaInfo and translates it into MeGUI's format.
// @TODO : Add all relevant languages found in MediaInfo.
void initLang()
{
	Languages[L"en"] = L"eng";
	Languages[L"ja"] = L"jpn";
	Languages[L"zh"] = L"chn";
	Languages[L"gr"] = L"ger";
	Languages[L"ru"] = L"rus";
	Languages[L"ko"] = L"kor";
}

/******************************************************************/
//	System Utility Functions
/******************************************************************/

/**
 Outputs a colored teext in the console.
 @param msg : Unicode message string to output.
 @param color : enum value of the color we want (0-255)
**/
void MsgColor(String msg, WORD color)
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	wcout << msg << endl;
	SetConsoleTextAttribute(hConsole, msg_norm);
}

/**
Outputs a colored teext in the console.
@param msg : Message string to output.
@param color : enum value of the color we want (0-255)
**/
void MsgColor(string msg, WORD color)
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	cout << msg << endl;
	SetConsoleTextAttribute(hConsole, msg_norm);
}

DWORD FindProcessId(const String & processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

bool TerminateProcess(DWORD processID)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
	if (!hProcess)
		return false;

	bool result = TerminateProcess(hProcess, 1);
	CloseHandle(hProcess);
	return result;
}

bool DoesFileExist(String filename)
{
	wifstream f(filename.data());
	return f.good();
}

/******************************************************************/
//	Video Info Functions
/******************************************************************/

/**
	Using MediaInfo, we obtain all relevant information on our media file.
	Since the library can figure out whether the file is valid or not for us,
	I can forgo having to do any crazy checks.

	@return The video file's infomration. Returns default/empty on error.
**/
videoFile getVideoInfo(const wchar_t* filepath, int jobNum)
{
	videoFile fileInfo;

	MediaInfo MI;
	MI.Open(filepath);

	// We NEED a video track. If none, return blank information
	if (!MI.Count_Get(Stream_Video))
	{
		MsgColor(L"ERROR: No video track found! (" + String(filepath).substr(fileInfo.filePath.find_last_of(L"\\") + 1, fileInfo.filePath.length() - fileInfo.filePath.find_last_of(L"\\") - 5) + L")", msg_erro);
		return videoFile();
	}

	fileInfo.filePath = filepath;
	fileInfo.fileName = fileInfo.filePath.substr(fileInfo.filePath.find_last_of(L"\\") + 1, fileInfo.filePath.length() - fileInfo.filePath.find_last_of(L"\\") - 5);
	fileInfo.parentDir = fileInfo.filePath.substr(0, fileInfo.filePath.find_last_of(L"\\") + 1);

	String tempParentDirName = fileInfo.parentDir.substr(0, fileInfo.parentDir.find_last_of(L"\\"));
	String tempParentofParentDir = tempParentDirName.substr(0, tempParentDirName.find_last_of(L"\\"));

	fileInfo.parentDirName = fileInfo.filePath.substr(tempParentofParentDir.length() + 1, tempParentDirName.length() - 1 - tempParentofParentDir.length());
	fileInfo.jobNum = jobNum;

	MsgColor(L"[Job " + to_wstring(fileInfo.jobNum) + L"/" + to_wstring(maxJobs) + L"] MKV Info: " + fileInfo.fileName, msg_info);

	if (bVerbose)
	{
		wcout << L"\r\n\r\nInform with Complete=false\r\n";
		MI.Option(L"Complete");
		wcout << MI.Inform() << endl;
	}

	// Video Info - We only handle the first video track if there are multiple.
	if (MI.Count_Get(Stream_Video))
	{

		fileInfo.videoTrack.trackNum = stoi(MI.Get(Stream_Video, 0, L"ID")) - 1;

		int resx = stoi(MI.Get(Stream_Video, 0, L"Width")),
			resy = stoi(MI.Get(Stream_Video, 0, L"Height"));
		if (resx && resy)
		{
			int cd = boost::integer::gcd(resx, resy);
			fileInfo.videoTrack.AR = to_wstring(float(resx) / float(resy));
			fileInfo.videoTrack.ARx = to_wstring(min(cd, (resx / cd)));
			fileInfo.videoTrack.ARy = to_wstring(min(cd, (resy / cd)));
		}
		
		fileInfo.videoTrack.bColorCorrect = fileInfo.videoTrack.bReencode = MI.Get(Stream_Video, 0, L"BitDepth") == L"10";

		if (MI.Get(Stream_General, 0, L"Format") == L"Matroska")
			fileInfo.videoTrack.extension = L"mkv";
		else
			fileInfo.videoTrack.extension = L"mp4";
	}

	// Audio Info - We generate track information for each audio track we find.
	for (int i = 0; i < MI.Count_Get(Stream_Audio); i++)
	{
		trackInfo tempTrack;
		tempTrack.language = Languages[MI.Get(Stream_Audio, i, L"Language")];
		if (tempTrack.language.empty())
		{
			MsgColor(L"Warning: Unknown audio language (" + MI.Get(Stream_Audio, i, L"Language") + L")", msg_warn);
			tempTrack.language = MI.Get(Stream_Audio, i, L"Language");
		}

		tempTrack.trackNum = stoi(MI.Get(Stream_Audio, i, L"ID")) - 1;
		tempTrack.filename = MI.Get(Stream_Audio, i, L"Title");

		if (MI.Get(Stream_Audio, i, L"Format") == L"AC-3")
		{
			tempTrack.extension = L"ac3";
		}
		else if (MI.Get(Stream_Audio, i, L"Format") == L"AAC")
		{
			tempTrack.extension = L"m4a";
		}
		else if (MI.Get(Stream_Audio, i, L"Format") == L"OGG")
		{
			tempTrack.extension = L"ogg";
			tempTrack.bReencode = true;
		}
		else if (MI.Get(Stream_Audio, i, L"Format") == L"FLAC")
		{
			tempTrack.extension = L"flac";
			tempTrack.bReencode = true;
		}
		else if (MI.Get(Stream_Audio, i, L"Format") == L"DTS")
		{
			tempTrack.extension = L"flac";
			tempTrack.bReencode = true;
		}
		else // In this case, we don't care what format it's in. Hopefully the decoder in MeGUI can understand it.
		{
			tempTrack.extension = L"m4a";
			tempTrack.bReencode = true;
			MsgColor(L"Warning: Unknown audio format (" + MI.Get(Stream_Audio, i, L"Format") + L"). Will try re-encode regardless if chosen.", msg_warn);
		}

		fileInfo.audioTracks.push_back(tempTrack);
	}

	// Subtitles Info - We generate track information for each subtitle track we find.
	for (int i = 0; i < MI.Count_Get(Stream_Text); i++)
	{
		trackInfo tempTrack;
		tempTrack.language = Languages[MI.Get(Stream_Text, i, L"Language")];
		if (tempTrack.language.empty())
		{
			MsgColor(L"Warning: Unknown subtitle language (" + MI.Get(Stream_Text, i, L"Language") + L")", msg_warn);
			tempTrack.language = L"";
		}

		tempTrack.trackNum = stoi(MI.Get(Stream_Text, i, L"ID")) - 1;
		tempTrack.filename = MI.Get(Stream_Text, i, L"Title");

		if (MI.Get(Stream_Text, i, L"Format") == L"ASS")
		{
			tempTrack.extension = L"ass";
		}
		else if (MI.Get(Stream_Text, i, L"Format") == L"srt")
		{
			tempTrack.extension = L"srt";
		}
		else if (MI.Get(Stream_Text, i, L"Format") == L"sup")
		{
			tempTrack.extension = L"sup";
		}
		else // In this case, we treat the subtitle as an ass file. 
		{
			tempTrack.extension = L"ass";
			MsgColor(L"Warning: Unknown subtitle format (" + MI.Get(Stream_Text, i, L"Format") + L"). Will try hard subbing regardless if chosen.", msg_warn);
			break;
		}

		fileInfo.subtitleTracks.push_back(tempTrack);
	}

	// Handling external subtitles if any exist.
	if (!fileInfo.subtitleTracks.size() && bUseConditionalExternalSubs)
	{
		trackInfo tempTrack;
		
		if (DoesFileExist(fileInfo.filePath.substr(0, fileInfo.filePath.length() - 4) + L".ass"))
			tempTrack.extension = L"ass";
		else if (DoesFileExist(fileInfo.filePath + L".srt"))
			tempTrack.extension = L"srt";
		else if (DoesFileExist(fileInfo.filePath + L".sup"))
			tempTrack.extension = L"sup";


		if (!tempTrack.extension.empty())
		{
			tempTrack.language = L"eng";
			tempTrack.trackNum = -1;
			tempTrack.filename = fileInfo.fileName + L"." + tempTrack.extension;
			tempTrack.bIsExternal = true;
		}

		fileInfo.subtitleTracks.push_back(tempTrack);
	}

	// Attachments - We generate track information for each attachment we find.
	if (!MI.Get(Stream_General, 0, L"Attachments").empty())
	{
		String tmpString = MI.Get(Stream_General, 0, L"Attachments");
		int pos1 = 0, nextPos = 0, i = 1;

		do {
			trackInfo tempTrack;
			nextPos = tmpString.find(L"/", pos1);
			if (nextPos != String::npos)
			{
				tempTrack.filename = tmpString.substr(pos1, nextPos - 1 - pos1);
				nextPos += (nextPos + 2 < tmpString.length() ? 2 : 0);
			}
			else
				tempTrack.filename = tmpString.substr(pos1, tmpString.length() - pos1);

			tempTrack.bIsFont = true;
			tempTrack.trackNum = i;
			pos1 = nextPos;
			i++;
			fileInfo.attachmentTracks.push_back(tempTrack);
		} while (nextPos != String::npos);

	}

	// Finish up!
	cleanFilename(fileInfo);
	selectTracks(fileInfo);
	return fileInfo;
}

// This utility function removes all the unneeded bits in the file name.
// This is generally going by Anime releases, but also works for scene releases as well.
void cleanFilename(videoFile &fileInfo)
{
	vector<int> pos;
	int found = -1;
	int pos1, pos2;

	fileInfo.outFileName = fileInfo.fileName;

	if (bCleanFilename)
	{
		pos.erase(pos.begin(), pos.end());

		// If we have nothing but .s for a filename, lets get rid of them.
		do {
			found = fileInfo.fileName.find(L".", found + 1);
			if (found != String::npos)
				pos.push_back(found);
		} while (found != String::npos);

		if (pos.size() > 3)
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"\\u002E"), L" ");

		// If we have nothing but _s for a filename, lets get rid of them.
		do {
			found = fileInfo.fileName.find(L"_", found + 1);
			if (found != String::npos)
				pos.push_back(found);
		} while (found != String::npos);

		if (pos.size() > 2)
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"_"), L" ");

		// Removing group is easy (In anime, its always in the beginning).
		if (fileInfo.outFileName[0] == '[')
		{
			int endGroup = int(fileInfo.outFileName.find_first_of(L"]")) + 2;
			fileInfo.outFileName = fileInfo.outFileName.substr(endGroup, int(fileInfo.outFileName.size()) - endGroup);
		}

		// Now to find and clear the rest using RegEx
		if (!customEpisodeRegex.empty() && regex_match(fileInfo.outFileName, wregex(customEpisodeRegex)))
		{ // Custom RegEx
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(customEpisodeRegex), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(customEpisodeRegex), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*) - (\\d\\d)\\b.*")))
		{ // [Title] - [00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) - (\\d\\d)\\b.*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)\\b(\\d\\d)\\b.*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*\\b)(\\d\\d)\\b.*")))
		{// [Title ][00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) (\\d\\d)\\b.*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)\\b(\\d\\d)\\b.*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*) - (\\d\\d)v\\d\\b.*")))
		{ // [Title] - [00]v[0]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) - (\\d\\d)v\\d\\b.*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)\\b(\\d\\d)v\\d\\b.*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*\\b)(\\d\\d)v\\d\\b.*")))
		{// [Title ][00]v[0]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) (\\d\\d)v\\d\\b.*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)\\b(\\d\\d)v\\d\\b.*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*)(S\\d\\dE\\d\\d).*")))
		{// [Title] S[00]E[00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) (S\\d\\dE\\d\\d).*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)(S\\d\\dE\\d\\d).*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*)(\\d\\d\\u002E\\d\\d).*")))
		{// [Title] S[00].E[00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) (\\d\\d\\u002E\\d\\d).*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)(\\d\\d\\u002E\\d\\d).*"), L"$1$2");
		}
		else if (regex_match(fileInfo.outFileName, wregex(L"(.*)(\\d\\d E\\d\\d).*")))
		{// [Title] S[00] E[00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(L"(.*) (\\d\\d E\\d\\d).*"), L"$1");
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(L"(.*)(\\d\\d E\\d\\d).*"), L"$1$2");
		}

		// And what about that data at the end? (Requires bAggressiveClean)
		if (bAggressiveClean && fileInfo.outFileName.find(L"[") != String::npos)
		{
			do {
				pos1 = fileInfo.outFileName.find(L"[");
				pos2 = fileInfo.outFileName.find(L"]") + 1;
				int remainingSize = (int(fileInfo.outFileName.size()) - pos2);
				fileInfo.outFileName = fileInfo.outFileName.substr(0, pos1) + fileInfo.outFileName.substr(pos2, remainingSize);
			} while (fileInfo.outFileName.find(L"[") != String::npos);
		}

		// Now for optional (parenthesis)
		if (bAggressiveClean && bCleanParenthesis && fileInfo.outFileName.find(L"(") != String::npos)
		{
			do {
				pos1 = fileInfo.outFileName.find(L"(");
				pos2 = fileInfo.outFileName.find(L")") + 1;
				int remainingSize = (int(fileInfo.outFileName.size()) - pos2);
				fileInfo.outFileName = fileInfo.outFileName.substr(0, pos1) + fileInfo.outFileName.substr(pos2, remainingSize);
			} while (fileInfo.outFileName.find(L"(") != String::npos);
		}
	}

	// Clean up end spaces if any
	while (!fileInfo.subDir.empty() && fileInfo.subDir[int(fileInfo.subDir.size()) - 1] == ' ')
		fileInfo.subDir.pop_back();
	while (!fileInfo.outFileName.empty() && fileInfo.outFileName[int(fileInfo.outFileName.size()) - 1] == ' ')
		fileInfo.outFileName.pop_back();

	// Finally, if the file is just numbers, affix the folder name if possible.
	try {
		// If this works, we only have numbers for a filename and subDir.
		stoi(fileInfo.outFileName);

		if (!fileInfo.parentDirName.empty())
		fileInfo.subDir = fileInfo.parentDirName;
		fileInfo.outFileName = fileInfo.parentDirName + L" - " + fileInfo.outFileName;
	}
	catch (...) { }

	if (fileInfo.subDir.empty())
		fileInfo.subDir = fileInfo.outFileName;
}

// Selects the tracks given the options set per video file.
void selectTracks(videoFile &fileInfo)
{
	bool bFound = false, bChoose = false;

	// Select Audio Track
	if (fileInfo.audioTracks.size() == 1)
		fileInfo.selectedAudioTrack = 0;
	else if (!fileInfo.audioTracks.size())
		fileInfo.selectedAudioTrack = -1;
	else
	{	
		// First, if we are choosing a relative forced audio track, handle it (if valid)
		if (bRelativeTrack && forcedAudioTrack > 0 && fileInfo.audioTracks.size() <= forcedAudioTrack)
			fileInfo.selectedAudioTrack = forcedAudioTrack - 1;
		else
		{
			for (int i = 0; i < fileInfo.audioTracks.size(); i++)
			{
				if (!bRelativeTrack && forcedAudioTrack == fileInfo.audioTracks[i].trackNum)
				{ // Set to the fourced track number
					fileInfo.selectedAudioTrack = i;
					break;
				}
				else if (fileInfo.audioTracks[i].language == defaultAudioLang && !bFound)
				{ // Set to designated language.
					fileInfo.selectedAudioTrack = i;
					bFound = true;
				}
				else if (fileInfo.audioTracks[i].language == defaultAudioLang && bFound)
				{ // We found a duplicate! If we have the option to select which to choose, continue as planned, otherwise warn the user.
					if (!bChooseTrack)
						MsgColor(L"WARNING: Found another audio track with the same language. Defaulting to first track.", msg_warn);
					else
						bChoose = true;
				}
				else if (i == int(fileInfo.audioTracks.size()) - 1 && !bFound)
				{ // Always default incase of errors or untagged tracks.
					MsgColor(L"WARNING: Could not find proper audio track! Defaulting to first track.", msg_warn);
					fileInfo.selectedAudioTrack = 0;
				}
			}
		}
	}

	// If we have multiple audio tracks that we couldn't figure out which to use (and the option bChooseTrack is set), ask the user for one.
	if (bChoose)
	{
		String inputStr;
		do {
			MsgColor("Please Choose from the following audio tracks:", msg_whit);

			for (int x = 0; x < fileInfo.audioTracks.size(); x++)
			{
				wcout << L"\t[" << to_wstring(x) << L"] Language: " << fileInfo.audioTracks[x].language << " Title: " << fileInfo.audioTracks[x].filename << endl;
			}

			wcout << "\t\t>> ";
			getline(wcin, inputStr);
			if (stoi(inputStr) < 0 || stoi(inputStr) >= fileInfo.audioTracks.size())
				MsgColor("Invalid track number!", msg_warn);
		} while (stoi(inputStr) < 0 && stoi(inputStr) >= fileInfo.audioTracks.size());
		fileInfo.selectedAudioTrack = stoi(inputStr);
	}

	// Select Subtitle Track (almost exactly the same as above!)
	bChoose = bFound = false;
	if (fileInfo.subtitleTracks.size() == 1)
		fileInfo.selecteSubtitleTrack = 0;
	else if (!fileInfo.subtitleTracks.size())
			fileInfo.selecteSubtitleTrack = -1;
	else
	{
		// First, if we are choosing a relative forced subtitle track, handle it (if valid)
		if (bRelativeTrack && forcedSubtitleTrack > 0 && fileInfo.subtitleTracks.size() <= forcedSubtitleTrack)
			fileInfo.selecteSubtitleTrack = forcedSubtitleTrack - 1;
		else {
			for (int i = 0; i < fileInfo.subtitleTracks.size(); i++)
			{
				if (!bRelativeTrack && forcedSubtitleTrack == fileInfo.subtitleTracks[i].trackNum)
				{ // Set to the fourced track number
					fileInfo.selecteSubtitleTrack = i;
					break;
				}
				else if (fileInfo.subtitleTracks[i].language == defaultSubtitleLang && !bFound)
				{ // Set to designated language.
					fileInfo.selecteSubtitleTrack = i;
					bFound = true;
				}
				else if (fileInfo.subtitleTracks[i].language == defaultSubtitleLang && bFound)
				{ // We found a duplicate! If we have the option to select which to choose, continue as planned, otherwise warn the user.
					if (!bChooseTrack)
						MsgColor(L"WARNING: Found another subtitle track with the same language. Defaulting to first track.", msg_warn);
					else
						bChoose = true;
				}
				else if (i == int(fileInfo.subtitleTracks.size()) - 1 && !bFound)
				{ // Always default incase of errors or untagged tracks.
					MsgColor(L"WARNING: Could not find proper subtitle track! Defaulting to first track.", msg_warn);
					fileInfo.selecteSubtitleTrack = 0;
				}
			}
		}

		// If we have multiple subtitles that we couldn't figure out which to use (and the option bChooseTrack is set), ask the user for one.
		if (bChoose)
		{
			String inputStr;
			do {
				MsgColor("Please Choose from the following subtitle tracks:", msg_whit);

				for (int x = 0; x < fileInfo.subtitleTracks.size(); x++)
				{
					wcout << L"\t[" << to_wstring(x) << L"] Language: " << fileInfo.subtitleTracks[x].language << " Title: " << fileInfo.subtitleTracks[x].filename << endl;
				}

				wcout << "\t\t>> ";
				getline(wcin, inputStr);
				if (stoi(inputStr) < 0 || stoi(inputStr) >= fileInfo.subtitleTracks.size())
					MsgColor("Invalid track number!", msg_warn);
			} while (stoi(inputStr) < 0 || stoi(inputStr) >= fileInfo.subtitleTracks.size());
			fileInfo.selecteSubtitleTrack = stoi(inputStr);
		}

	}

	// One final sanity check. If we choose a SUP subtitle, make sure we have the proper plugin for it!
	if (fileInfo.selecteSubtitleTrack > 0 && fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension == L"sup" && !DoesFileExist(MeGUIDir + L"\\tools\\avisynth_plugin\\SupTitle.dll"))
	{
		MsgColor(L"ERROR: SUP subtitles requires SupTitle plugin in MeGUI's AviSynth plugin folder! Skipping subtitle.", msg_erro);
		fileInfo.selecteSubtitleTrack = -1;
	}
}


/******************************************************************/
//	Video Extraction Functions
/******************************************************************/

// Uses MeGUI's mkvextract found in its tools to extract all relevent files (audio, subtitles, attachments)
void extractMKV(videoFile fileInfo)
{
	MsgColor(L"[Job " + to_wstring(fileInfo.jobNum) + L"/" + to_wstring(maxJobs) + L"] MKV Extract: " + fileInfo.outFileName, msg_info);

	// This is a failsafe. At a minimum we require an audio track to extract to continue.
	if (!bExtract264 && fileInfo.selectedAudioTrack < 0 && fileInfo.selecteSubtitleTrack < 0 && (!bDoAttachments || (bDoAttachments && !fileInfo.attachmentTracks.size())))
	{
		MsgColor(L"WARNING: Nothing to extract?", msg_warn);
		return;
	}

	// Building the command string
	String cmdstr = MeGUIDir + L"tools\\mkvmerge\\mkvextract.exe \"" + fileInfo.filePath + L"\" ";

	if (bExtract264)
		cmdstr += L"tracks " + to_wstring(fileInfo.videoTrack.trackNum) + L":\"" + WorkDir + fileInfo.outFileName + L".264\" ";

	if (fileInfo.selectedAudioTrack >= 0)
		cmdstr += L"tracks " + to_wstring(fileInfo.audioTracks[fileInfo.selectedAudioTrack].trackNum) + L":\"" + WorkDir + fileInfo.outFileName + L"." + fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension + L"\" ";

	if (fileInfo.selecteSubtitleTrack >= 0 && !fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].bIsExternal)
		cmdstr += L"tracks " + to_wstring(fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].trackNum) + L":\"" + WorkDir + fileInfo.outFileName + L"." + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension + L"\" ";

	if (bDoAttachments && fileInfo.attachmentTracks.size())
	{
		bExtractedAttachments = true;
		for each (trackInfo atch in fileInfo.attachmentTracks)
			cmdstr += L"attachments " + to_wstring(atch.trackNum) + L":\"" + WorkDir + L"Fonts\\" + atch.filename + L"\" ";
	}

	_wsystem(cmdstr.data());

	// Extranal subtitle copying
	if (fileInfo.selecteSubtitleTrack >= 0 && fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].bIsExternal)
	{
		try {
			MsgColor(L"Copying subtitle (" + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].filename + L") to WorkDir", msg_info);
			CopyFile(const_cast<LPWSTR>(String(fileInfo.parentDir + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].filename).c_str()), const_cast<LPWSTR>(String(WorkDir + fileInfo.outFileName + L"." + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension).c_str()), true);
		}
		catch (std::exception e)
		{
			MsgColor("Copying subtitle ERROR: " + string(e.what()), msg_erro);
		}
	}
}

// Uses MeGUI's mp4box found in its tools to extract all relevent files (audio and subtitles)
//	Note: MP4 does not have attachment support for fonts.
void extractMP4(videoFile fileInfo)
{
	MsgColor(L"[Job " + to_wstring(fileInfo.jobNum) + L"/" + to_wstring(maxJobs) + L"] MP4 Extract: " + fileInfo.outFileName, msg_info);

	if (!bExtract264 && fileInfo.selectedAudioTrack < 0 && fileInfo.selecteSubtitleTrack < 0 && (!bDoAttachments || (bDoAttachments && !fileInfo.attachmentTracks.size())))
	{
		MsgColor(L"WARNING: Nothing to extract?", msg_warn);
		return;
	}

	// Building the command string
	String cmdstr = MeGUIDir + L"tools\\mp4box\\mp4box.exe \"" + fileInfo.filePath + L"\" ";

	if (bExtract264)
		cmdstr += L"-raw " + to_wstring(fileInfo.videoTrack.trackNum + 1) + L":output=\"" + WorkDir + fileInfo.outFileName + L".264\" ";

	if (fileInfo.selectedAudioTrack >= 0)
		cmdstr += L"-raw " + to_wstring(fileInfo.audioTracks[fileInfo.selectedAudioTrack].trackNum + 1) + L":output=\"" + WorkDir + fileInfo.outFileName + L"." + fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension + L"\" ";

	if (fileInfo.selecteSubtitleTrack >= 0)
		cmdstr += L"-raw " + to_wstring(fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].trackNum + 1) + L":output=\"" + WorkDir + fileInfo.outFileName + L"." + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension + L"\" ";

	_wsystem(cmdstr.data());

	// Extranal subtitle copying
	if (fileInfo.selecteSubtitleTrack >= 0 && fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].bIsExternal)
	{
		try {
			MsgColor(L"Copying subtitle (" + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].filename + L") to WorkDir", msg_info);
			wifstream sourceFile(fileInfo.parentDir + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].filename, ios::binary);
			wofstream destinationFile(WorkDir + fileInfo.outFileName + L"." + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension, ios::binary);

			destinationFile << sourceFile.rdbuf();
			destinationFile.close();
			sourceFile.close();
		}
		catch (std::exception e)
		{
			MsgColor("Copying subtitle ERROR: " + string(e.what()), msg_erro);
		}
	}
}

/******************************************************************/
//	MeGUI Job Functions
/******************************************************************/

// Clears MeGUI's joblists.xml of all previous listed jobs.
// We do not need a full on xml parser as we're simply doing file editing.
void clearMeGUIJobs()
{
	wifstream myJobListRead(String(MeGUIDir + L"joblists.xml"));
	String curLine;
	vector<String> jobListRAW;

	// First we read in the file line by line for us to parse.
	if (myJobListRead.is_open())
	{
		while (getline(myJobListRead, curLine))
			jobListRAW.push_back(curLine);

		myJobListRead.close();
	}
	else
	{
		MsgColor(L"ERROR: Unable to open joblist.xml!", msg_erro);
		return;
	}

	MsgColor(L"Clearing MeGUI Jobs", msg_info);

	// Now to rewrite the file.
	wofstream myJobListWrite(String(MeGUIDir + L"joblists.xml"));
	if (myJobListWrite.is_open())
	{
		try {
			int x;
			for (x = 0; x <= int(jobListRAW.size()) - 1; x++)
			{ // We are looking for the following: <String>[jobname].xml</String>
				if (bClearMeGUIJobs && jobListRAW[x].find(L"String") != String::npos)
				{ // We found one. Delete line and continue on.
					String file = jobListRAW[x].substr(jobListRAW[x].find(L">") + 1, jobListRAW[x].find(L"</") - (jobListRAW[x].find(L">") + 1));
					if (_wremove(String(MeGUIDir + L"\\jobs\\" + file + L".xml").c_str()) == 0)
						wcout << L"Removed: MeGUI\\jobs\\" + file + L".xml\n";

					jobListRAW.erase(jobListRAW.begin() + x);
					x--;
					continue;
				}
				else if (bClearMeGUIJobs && jobListRAW[x].find(L"<mainJobList>") != String::npos)
				{ // When we find <mainJobList>, replace it with <mainJobList /> to complete the cleaning.
					jobListRAW[x] = regex_replace(jobListRAW[x], wregex(L"mainJobList"), L"mainJobList /");
				}
				else if (bClearMeGUIJobs && jobListRAW[x].find(L"</mainJobList>") != String::npos)
				{ // Remove the trailing </mainJobList>
					jobListRAW.erase(jobListRAW.begin() + x);
					x--;
					continue;
				}

				// Finally, write the file.
				myJobListWrite << jobListRAW[x] << endl;
			}

			wcout << "Cleaned JobList." << endl;
		}
		catch (std::exception e)
		{
			MsgColor("Clearing MeGUI Jobs ERROR: " + string(e.what()), msg_erro);
		}
		myJobListWrite.close();
	}
	else
	{
		MsgColor(L"ERROR: Unable to open joblist.xml!", msg_erro);
		return;
	}
}

// Creates the corrosponding job lists to re-encode the video and audio files with hard subtitles (if any)
void createMeGUIJobs(videoFile fileInfo)
{
	String lJobTag = L"\t\t<string>";
	String rJobTag = L"</string>\n";
	String tempList = L"";

	// Failsafe. If the video file is in the format we need without any subtitles to work with, we do nothing.
	if (!fileInfo.videoTrack.bReencode && !fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode && fileInfo.selecteSubtitleTrack < 0)
	{
		MsgColor(L"[Job " + to_wstring(fileInfo.jobNum) + L"/" + to_wstring(maxJobs) + L"] Skipping MeGUI Jobs: " + fileInfo.outFileName + L"\nVideo file does not require a re-encode!", msg_warn);
		return;
	}

	MsgColor(L"[Job " + to_wstring(fileInfo.jobNum) + L"/" + to_wstring(maxJobs) + L"] Creating MeGUI Jobs: " + fileInfo.outFileName, msg_info);

	// AVS Script - Uses Booost's format functionality like old school C
	wofstream myAVSFile(String(WorkDir + fileInfo.outFileName + L".avs"));
	if (myAVSFile.is_open())
	{
		try {
			if (!bExtract264) // VERY IMPORTANT! Without YUV20P8, 10bit encodes will create garbage data that 8bit cannot understand!
				myAVSFile << wformat(AVSTemplate) % MeGUIDir % fileInfo.filePath % (fileInfo.videoTrack.bColorCorrect ? L", format=\"YUV420P8\"" : L"");
			else
				myAVSFile << wformat(AVSTemplate_264) % MeGUIDir % fileInfo.outFileName % WorkDir % (fileInfo.videoTrack.bColorCorrect ? L", format=\"YUV420P8\"" : L"");

			// Hard coding subtitles are handled here. Note the use of a third-party plugin for SUP files.
			if (fileInfo.selecteSubtitleTrack != -1 && (fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension == L"ass" || fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension == L"srt"))
				myAVSFile << wformat(AVSTemplate_Subs) % MeGUIDir % fileInfo.outFileName % WorkDir % fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension;
			else if (fileInfo.selecteSubtitleTrack != -1)
				myAVSFile << wformat(AVSTemplate_Sups) % MeGUIDir % fileInfo.outFileName % WorkDir % fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension;
			wcout << L"Created AVS Script." << endl;
		}
		catch (std::exception e)
		{
			MsgColor("AVS ERROR: " + string(e.what()), msg_erro);
		}
		myAVSFile.close();
	}
	else
	{
		MsgColor("ERROR: Unable to open AVS file!", msg_erro);
		return;
	}

	// 264 Job Script - Uses the AVS file generated from above.
	wofstream myVideoJob(String(MeGUIDir + L"jobs\\" + fileInfo.outFileName + L" - 264.xml"));
	if (myVideoJob.is_open())
	{
		try {
			myVideoJob << wformat(videoJobTemplate) % fileInfo.outFileName % WorkDir % WorkDir % fileInfo.videoTrack.AR % fileInfo.videoTrack.ARx % fileInfo.videoTrack.ARy;
			wcout << "Created 264 Job." << endl;
			tempList += lJobTag + fileInfo.outFileName + L" - 264" + rJobTag;
			jobNumber++;
		}
		catch (std::exception e)
		{
			MsgColor("264 XML Job ERROR: " + string(e.what()), msg_erro);
		}
		myVideoJob.close();
	}
	else
	{
		MsgColor("ERROR: Unable to open 264 XML file!", msg_erro);
		return;
	}

	// Audio Job Script - If there is a reason to re-encode, we do so. Otherwise, this is skipped.
	if (fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode)
	{
		wofstream myAudioJob(String(MeGUIDir + L"jobs\\" + fileInfo.outFileName + L" - AC3.xml"));
		if (myAudioJob.is_open())
		{
			try {
				myAudioJob << wformat(audioJobTemplate) % fileInfo.outFileName % WorkDir % fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension;
				wcout << "Created AC3 Job." << endl;
				tempList += lJobTag + fileInfo.outFileName + L" - AC3" + rJobTag;
				jobNumber++;
			}
			catch (std::exception e)
			{
				MsgColor("AC3 XML Job ERROR: " + string(e.what()), msg_erro);
			}
			myAudioJob.close();
		}
		else
		{
			MsgColor(L"ERROR: Unable to open AC3 XML file!", msg_erro);
			return;
		}
	}

	// Mux Job Script - We wrap it up with this file, putting all the encoded files together. It also handles the deletion of all extracted files (but not fonts)
	wofstream myMuxJob(String(MeGUIDir + L"jobs\\" + fileInfo.outFileName + L" - MKV.xml"));
	if (myMuxJob.is_open())
	{
		try {
			String outDir = OutputDir;
			if (bCreateSubDir && !fileInfo.subDir.empty())
				outDir += fileInfo.subDir + L"\\";

			String framerate = L" xsi:nil=\"true\" />";

			if (!fileInfo.videoTrack.framerate.empty())
				framerate = L">" + fileInfo.videoTrack.framerate + L"</Framerate>";
			
			if (!bExtract264)
				myMuxJob << wformat(muxJobTemplate) % fileInfo.outFileName % WorkDir % WorkDir % outDir % (fileInfo.selectedAudioTrack >= 0 ? (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension : L"ac3") : L"m4a") % framerate % to_wstring(jobNumber) % to_wstring(jobNumber - (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? 2 : 1)) % fileInfo.filePath;
			else
				myMuxJob << wformat(muxJobTemplate_264) % fileInfo.outFileName % WorkDir % WorkDir % outDir % (fileInfo.selectedAudioTrack >= 0 ? (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension : L"ac3") : L"m4a") % framerate;

			wcout << L"Created Mux Job." << endl;
			tempList += lJobTag + fileInfo.outFileName + L" - MKV" + rJobTag;
			jobNumber++;
		}
		catch (std::exception e)
		{
			MsgColor("MKV XML Job ERROR: " + string(e.what()), msg_erro);
		}
		myMuxJob.close();
	}
	else
	{
		MsgColor(L"ERROR: Unable to open MKV XML file!", msg_erro);
		return;
	}

	// Add entry to master job List. Again, no use of an XML parser as we're only editing a single line.
	wifstream myJobListRead(String(MeGUIDir + L"joblists.xml"));
	String curLine;
	vector<String> jobListRAW;

	// Read in the file into a vector.
	if (myJobListRead.is_open())
	{
		while (getline(myJobListRead, curLine))
			jobListRAW.push_back(curLine);

		myJobListRead.close();
	}
	else
	{
		MsgColor(L"ERROR: Unable to open joblists.xml to read!", msg_erro);
		return;
	}

	wofstream myJobListWrite(String(MeGUIDir + L"joblists.xml"));
	if (myJobListWrite.is_open())
	{
		bool bIsOutFilenameNum = false;
		try {
			stoi(fileInfo.outFileName);
			bIsOutFilenameNum = true;
		}
		catch (...) { }

		try {
			if (jobListRAW.size() == 0)
				myJobListWrite << wformat(jobListTemplate) % tempList;
			else
			{
				int x;
				for (x = 0; x <= int(jobListRAW.size()) - 1; x++)
				{
					if ((jobListRAW[x].find(fileInfo.outFileName) != String::npos && !bIsOutFilenameNum) || (bIsOutFilenameNum && jobListRAW[x].find(fileInfo.outFileName + L" - ") != String::npos))
					{ // If we find any duplicates, delete them.
						jobListRAW.erase(jobListRAW.begin() + x);
						x--;
						continue;
					}
					else if (jobListRAW[x].find(L"<mainJobList />") != String::npos)		// Replace the empty list with our own.
						jobListRAW[x] = L"<mainJobList>\n" + tempList + L"</mainJobList>";
					else if (jobListRAW[x].find(L"</mainJobList>") != String::npos)			// Add to the end of the list.
						jobListRAW[x] = tempList + jobListRAW[x];

					myJobListWrite << jobListRAW[x] << endl;
				}
			}
			wcout << "Created JobList." << endl;
		}
		catch (std::exception e)
		{
			MsgColor("Writing MeGUI Jobs ERROR: " + string(e.what()), msg_erro);
		}
		myJobListWrite.close();
	}
	else
	{
		MsgColor(L"ERROR: Unable to open joblist.xml!", msg_erro);
		return;
	}
}