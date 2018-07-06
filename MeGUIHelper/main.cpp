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

#include <boost/integer/common_factor_rt.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
namespace po = boost::program_options;

#pragma warning(disable : 4996)

using namespace std;
using namespace boost;

String MeGUIDir = __T("");
String WorkDir = __T("");
String OutputDir = __T("");
String defaultAudioLang = __T("jpn");
String defaultSubtitleLang = __T("eng");
String customEpisodeRegex = __T("");
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

const String AVSTemplate = __T("LoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\lsmash\\LSMASHSource.dll\")\nLWLibavVideoSource(\"%1%\")\n");
const String AVSTemplate_264 = __T("LoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\ffms\\ffms2.dll\")\nFFVideoSource(\"%2%%1%.264\")\n");
const String AVSTemplate_Subs = __T("\nLoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\avisynth_plugin\\VSFilter.dll\")\nTextSub(\"%2%%1%.%3%\", 1)");
const String AVSTemplate_Sups = __T("\nLoadPlugin(\"E:\\Downloads\\[Media]\\MeGUI-2836-32\\tools\\avisynth_plugin\\SupTitle.dll\")\nTextSub(\"%2%%1%.%3%\")");
const String videoJobTemplate = __T("<?xml version=\"1.0\" encoding=\"windows-1252\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n\t<EncodingSpeed />\n\t<Job xsi:type=\"VideoJob\">\n\t<Input>%2%%1%.avs</Input>\n\t<Output>%3%%1%-1.264</Output>\n\t<FilesToDelete />\n\t<Zones />\n\t<DAR>\n\t\t<AR>%4%</AR>\n\t\t<X>%5%</X>\n\t\t<Y>%6%</Y>\n\t</DAR>\n\t<Settings xsi:type=\"x264Settings\">\n\t\t<VideoEncodingType>quality</VideoEncodingType>\n\t\t<BitrateQuantizer>20</BitrateQuantizer>\n\t\t<KeyframeInterval>250</KeyframeInterval>\n\t\t<NbBframes>3</NbBframes>\n\t\t<MinQuantizer>0</MinQuantizer>\n\t\t<MaxQuantizer>81</MaxQuantizer>\n\t\t<V4MV>false</V4MV>\n\t\t<QPel>false</QPel>\n\t\t<Trellis>false</Trellis>\n\t\t<CreditsQuantizer>40</CreditsQuantizer>\n\t\t<Logfile>%3%%1%.stats</Logfile>\n\t\t<VideoName />\n\t\t<CustomEncoderOptions />\n\t\t<MaxNumberOfPasses>3</MaxNumberOfPasses>\n\t\t<NbThreads>0</NbThreads>\n\t\t<x264PresetLevel>medium</x264PresetLevel>\n\t\t<x264PsyTuning>NONE</x264PsyTuning>\n\t\t<QuantizerCRF>20.0</QuantizerCRF>\n\t\t<InterlacedMode>progressive</InterlacedMode>\n\t\t<TargetDeviceXML>12</TargetDeviceXML>\n\t\t<BlurayCompatXML>False</BlurayCompatXML>\n\t\t<NoDCTDecimate>false</NoDCTDecimate>\n\t\t<PSNRCalculation>false</PSNRCalculation>\n\t\t<NoFastPSkip>false</NoFastPSkip>\n\t\t<NoiseReduction>0</NoiseReduction>\n\t\t<NoMixedRefs>false</NoMixedRefs>\n\t\t<X264Trellis>1</X264Trellis>\n\t\t<NbRefFrames>3</NbRefFrames>\n\t\t<AlphaDeblock>0</AlphaDeblock>\n\t\t<BetaDeblock>0</BetaDeblock>\n\t\t<SubPelRefinement>7</SubPelRefinement>\n\t\t<MaxQuantDelta>4</MaxQuantDelta>\n\t\t<TempQuantBlur>0</TempQuantBlur>\n\t\t<BframePredictionMode>1</BframePredictionMode>\n\t\t<VBVBufferSize>31250</VBVBufferSize>\n\t\t<VBVMaxBitrate>31250</VBVMaxBitrate>\n\t\t<METype>1</METype>\n\t\t<MERange>16</MERange>\n\t\t<MinGOPSize>25</MinGOPSize>\n\t\t<IPFactor>1.4</IPFactor>\n\t\t<PBFactor>1.3</PBFactor>\n\t\t<ChromaQPOffset>0</ChromaQPOffset>\n\t\t<VBVInitialBuffer>0.9</VBVInitialBuffer>\n\t\t<BitrateVariance>1.0</BitrateVariance>\n\t\t<QuantCompression>0.6</QuantCompression>\n\t\t<TempComplexityBlur>20</TempComplexityBlur>\n\t\t<TempQuanBlurCC>0.5</TempQuanBlurCC>\n\t\t<SCDSensitivity>40</SCDSensitivity>\n\t\t<BframeBias>0</BframeBias>\n\t\t<PsyRDO>1.0</PsyRDO>\n\t\t<PsyTrellis>0</PsyTrellis>\n\t\t<Deblock>true</Deblock>\n\t\t<Cabac>true</Cabac>\n\t\t<UseQPFile>false</UseQPFile>\n\t\t<WeightedBPrediction>true</WeightedBPrediction>\n\t\t<WeightedPPrediction>2</WeightedPPrediction>\n\t\t<NewAdaptiveBFrames>1</NewAdaptiveBFrames>\n\t\t<x264BFramePyramid>2</x264BFramePyramid>\n\t\t<x264GOPCalculation>1</x264GOPCalculation>\n\t\t<ChromaME>true</ChromaME>\n\t\t<MacroBlockOptions>3</MacroBlockOptions>\n\t\t<P8x8mv>true</P8x8mv>\n\t\t<B8x8mv>true</B8x8mv>\n\t\t<I4x4mv>true</I4x4mv>\n\t\t<I8x8mv>true</I8x8mv>\n\t\t<P4x4mv>false</P4x4mv>\n\t\t<AdaptiveDCT>true</AdaptiveDCT>\n\t\t<SSIMCalculation>false</SSIMCalculation>\n\t\t<StitchAble>false</StitchAble>\n\t\t<QuantizerMatrix>Flat (none)</QuantizerMatrix>\n\t\t<QuantizerMatrixType>0</QuantizerMatrixType>\n\t\t<DeadZoneInter>21</DeadZoneInter>\n\t\t<DeadZoneIntra>11</DeadZoneIntra>\n\t\t<X26410Bits>false</X26410Bits>\n\t\t<OpenGop>False</OpenGop>\n\t\t<X264PullDown>0</X264PullDown>\n\t\t<SampleAR>0</SampleAR>\n\t\t<ColorMatrix>0</ColorMatrix>\n\t\t<ColorPrim>0</ColorPrim>\n\t\t<Transfer>0</Transfer>\n\t\t<AQmode>1</AQmode>\n\t\t<AQstrength>1.0</AQstrength>\n\t\t<QPFile />\n\t\t<Range>auto</Range>\n\t\t<x264AdvancedSettings>true</x264AdvancedSettings>\n\t\t<Lookahead>40</Lookahead>\n\t\t<NoMBTree>true</NoMBTree>\n\t\t<ThreadInput>true</ThreadInput>\n\t\t<NoPsy>false</NoPsy>\n\t\t<Scenecut>true</Scenecut>\n\t\t<Nalhrd>0</Nalhrd>\n\t\t<X264Aud>false</X264Aud>\n\t\t<X264SlowFirstpass>false</X264SlowFirstpass>\n\t\t<PicStruct>false</PicStruct>\n\t\t<FakeInterlaced>false</FakeInterlaced>\n\t\t<NonDeterministic>false</NonDeterministic>\n\t\t<SlicesNb>0</SlicesNb>\n\t\t<MaxSliceSyzeBytes>0</MaxSliceSyzeBytes>\n\t\t<MaxSliceSyzeMBs>0</MaxSliceSyzeMBs>\n\t\t<Profile>2</Profile>\n\t\t<AVCLevel>L_41</AVCLevel>\n\t\t<TuneFastDecode>false</TuneFastDecode>\n\t\t<TuneZeroLatency>false</TuneZeroLatency>\n\t</Settings>\n\t</Job>\n\t<RequiredJobNames />\n\t<EnabledJobNames>\n\t<string>job%8% - %1% - MKV</string>\n\t</EnabledJobNames>\n\t<Name>job%7% - %1% - 264</Name>\n\t<Status>WAITING</Status>\n\t<Start>0001-01-01T00:00:00</Start>\n\t<End>0001-01-01T00:00:00</End>\n</TaggedJob>");
const String audioJobTemplate = __T("<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"AudioJob\">\n    <Input>%2%%1%.%3%</Input>\n    <Output>%2%%1%.ac3</Output>\n    <FilesToDelete />\n    <CutFile />\n    <Settings xsi:type=\"AC3Settings\">\n      <PreferredDecoderString>LWLibavAudioSource</PreferredDecoderString>\n      <DownmixMode>KeepOriginal</DownmixMode>\n      <BitrateMode>CBR</BitrateMode>\n      <Bitrate>384</Bitrate>\n      <AutoGain>false</AutoGain>\n      <SampleRateType>deprecated</SampleRateType>\n      <SampleRate>KeepOriginal</SampleRate>\n      <TimeModification>KeepOriginal</TimeModification>\n      <ApplyDRC>false</ApplyDRC>\n      <Normalize>100</Normalize>\n      <CustomEncoderOptions />\n    </Settings>\n    <Delay>0</Delay>\n    <SizeBytes>0</SizeBytes>\n    <BitrateMode>CBR</BitrateMode>\n  </Job>\n  <RequiredJobNames />\n  <EnabledJobNames />\n  <Name>job%4% - %1% - AC3</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>");
const String muxJobTemplate =	__T("<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"MuxJob\">\n    <Input>%3%%1%-1.264</Input>\n    <Output>%4%%1%.mkv</Output>\n    <FilesToDelete>\n      <string>%2%%1%.264</string>\n      <string>>%9%.lwi</string>\n      <string>%3%%1%-1.264</string>\n      <string>%2%%1%.avs</string>\n      <string>%3%%1%.%5%</string>\n    </FilesToDelete>\n    <ContainerTypeString>MKV</ContainerTypeString>\n    <Codec />\n    <NbOfBFrames>0</NbOfBFrames>\n    <NbOfFrames>0</NbOfFrames>\n    <Bitrate>0</Bitrate>\n    <Overhead>4.3</Overhead>\n    <Settings>\n      <MuxedInput />\n      <MuxedOutput>%4%%1%.mkv</MuxedOutput>\n      <VideoInput>%3%%1%-1.264</VideoInput>\n      <AudioStreams>\n        <MuxStream>\n          <path>%3%%1%.%5%</path>\n          <delay>0</delay>\n          <bDefaultTrack>false</bDefaultTrack>\n          <bForceTrack>false</bForceTrack>\n          <language>Japanese</language>\n          <name />\n        </MuxStream>\n      </AudioStreams>\n      <SubtitleStreams />\n      <Framerate%6%\n      <ChapterInfo>\n        <Title />\n        <SourceFilePath />\n        <SourceType />\n        <FramesPerSecond>0</FramesPerSecond>\n        <TitleNumber>0</TitleNumber>\n        <PGCNumber>0</PGCNumber>\n        <AngleNumber>0</AngleNumber>\n        <Chapters />\n        <DurationTicks>0</DurationTicks>\n      </ChapterInfo>\n      <Attachments />\n      <SplitSize xsi:nil=\"true\" />\n      <DAR xsi:nil=\"true\" />\n      <DeviceType>Standard</DeviceType>\n      <VideoName>%1%</VideoName>\n      <MuxAll>false</MuxAll>\n    </Settings>\n    <MuxType>MKVMERGE</MuxType>\n  </Job>\n  <RequiredJobNames>\n  <string>job%8% - %1% - 264</string>\n  </RequiredJobNames>\n  <EnabledJobNames />\n  <Name>job%7% - %1% - MKV</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>");
const String muxJobTemplate_264 =	__T("<?xml version=\"1.0\"?>\n<TaggedJob xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <EncodingSpeed />\n  <Job xsi:type=\"MuxJob\">\n    <Input>%3%%1%-1.264</Input>\n    <Output>%4%%1%.mkv</Output>\n    <FilesToDelete>\n      <string>%2%%1%.264</string>\n      <string>%2%%1%.264.ffindex</string>\n      <string>%3%%1%-1.264</string>\n      <string>%2%%1%.avs</string>\n      <string>%3%%1%.%5%</string>\n    </FilesToDelete>\n    <ContainerTypeString>MKV</ContainerTypeString>\n    <Codec />\n    <NbOfBFrames>0</NbOfBFrames>\n    <NbOfFrames>0</NbOfFrames>\n    <Bitrate>0</Bitrate>\n    <Overhead>4.3</Overhead>\n    <Settings>\n      <MuxedInput />\n      <MuxedOutput>%4%%1%.mkv</MuxedOutput>\n      <VideoInput>%3%%1%-1.264</VideoInput>\n      <AudioStreams>\n        <MuxStream>\n          <path>%3%%1%.%5%</path>\n          <delay>0</delay>\n          <bDefaultTrack>false</bDefaultTrack>\n          <bForceTrack>false</bForceTrack>\n          <language>Japanese</language>\n          <name />\n        </MuxStream>\n      </AudioStreams>\n      <SubtitleStreams />\n      <Framerate%6%\n      <ChapterInfo>\n        <Title />\n        <SourceFilePath />\n        <SourceType />\n        <FramesPerSecond>0</FramesPerSecond>\n        <TitleNumber>0</TitleNumber>\n        <PGCNumber>0</PGCNumber>\n        <AngleNumber>0</AngleNumber>\n        <Chapters />\n        <DurationTicks>0</DurationTicks>\n      </ChapterInfo>\n      <Attachments />\n      <SplitSize xsi:nil=\"true\" />\n      <DAR xsi:nil=\"true\" />\n      <DeviceType>Standard</DeviceType>\n      <VideoName>%1%</VideoName>\n      <MuxAll>false</MuxAll>\n    </Settings>\n    <MuxType>MKVMERGE</MuxType>\n  </Job>\n  <RequiredJobNames>\n  <string>job%8% - %1% - 264</string>\n  </RequiredJobNames>\n  <EnabledJobNames />\n  <Name>job%7% - %1% - MKV</Name>\n  <Status>WAITING</Status>\n  <Start>0001-01-01T00:00:00</Start>\n  <End>0001-01-01T00:00:00</End>\n</TaggedJob>");
const String jobListTemplate = __T("<?xml version=\"1.0\"?>\n<JobListSerializer xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n\t<mainJobList>%1%</mainJobList>\n\t<workersAndTheirJobLists>\n\t\t<PairOfStringListOfString>\n\t\t\t<fst>Worker 1</fst>\n\t\t\t<snd />\n\t\t</PairOfStringListOfString>\n\t</workersAndTheirJobLists>\n</JobListSerializer>");

struct trackInfo {
	String filename = __T("");
	String extension = __T("");
	String language = __T("");
	String framerate = __T("23.976");
	String AR = __T("1.7777777777777777777777777778");
	String ARx = __T("16");
	String ARy = __T("9");
	int trackNum = -1;
	bool bReencode = false;
	bool bIsFont = false;
};

struct videoFile {
	String filePath = __T("");
	String fileName = __T("");
	String parentDir = __T("");
	String outFileName = __T("");
	String subDir = __T("");

	trackInfo videoTrack;
	vector<trackInfo> audioTracks;
	vector<trackInfo> subtitleTracks;
	vector<trackInfo> attachmentTracks;
	int selectedAudioTrack = 0;
	int selecteSubtitleTrack = -1;
	int jobNum = -1;
};

enum cmd_color{
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


void MsgColor(String msg, int color)
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	wcout << msg << endl;
	SetConsoleTextAttribute(hConsole, msg_norm);
}

void MsgColor(string msg, int color)
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	cout << msg << endl;
	SetConsoleTextAttribute(hConsole, msg_norm);
}

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
			found = fileInfo.fileName.find(__T("."), found + 1);
			if (found != String::npos)
				pos.push_back(found);
		} while (found != String::npos);
		
		if (pos.size() > 3)
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("\\u002E")), __T(" "));

		// If we have nothing but _s for a filename, lets get rid of them.
		do {
			found = fileInfo.fileName.find(__T("_"), found + 1);
			if (found != String::npos)
				pos.push_back(found);
		} while (found != String::npos);

		if (pos.size() > 2)
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("_")), __T(" "));

		// Removing group is easy.
		if (fileInfo.outFileName[0] == '[')
		{
			int endGroup = int(fileInfo.outFileName.find_first_of(__T("]"))) + 2;
			fileInfo.outFileName = fileInfo.outFileName.substr(endGroup, int(fileInfo.outFileName.size()) - endGroup);
		}

		// But removing the rest?
		if (!customEpisodeRegex.empty() && regex_match(fileInfo.outFileName, wregex(customEpisodeRegex)))
		{ // Custom RegEx
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(customEpisodeRegex), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(customEpisodeRegex), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*) - (\\d\\d)\\b.*"))))
		{ // [Title] - [00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) - (\\d\\d)\\b.*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)\\b(\\d\\d)\\b.*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*\\b)(\\d\\d)\\b.*"))))
		{// [Title ][00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) (\\d\\d)\\b.*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)\\b(\\d\\d)\\b.*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*) - (\\d\\d)v\\d\\b.*"))))
		{ // [Title] - [00]v0
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) - (\\d\\d)v\\d\\b.*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)\\b(\\d\\d)v\\d\\b.*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*\\b)(\\d\\d)v\\d\\b.*"))))
		{// [Title ][00]v0
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) (\\d\\d)v\\d\\b.*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)\\b(\\d\\d)v\\d\\b.*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*)(S\\d\\dE\\d\\d).*"))))
		{// [Title] [S00E00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) (S\\d\\dE\\d\\d).*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)(S\\d\\dE\\d\\d).*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*)(\\d\\d\\u002E\\d\\d).*"))))
		{// [Title] [S00.E00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) (\\d\\d\\u002E\\d\\d).*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)(\\d\\d\\u002E\\d\\d).*")), __T("$1$2"));
		}
		else if (regex_match(fileInfo.outFileName, wregex(__T("(.*)(\\d\\d E\\d\\d).*"))))
		{// [Title] [S00 E00]
			fileInfo.subDir = regex_replace(fileInfo.outFileName, wregex(__T("(.*) (\\d\\d E\\d\\d).*")), __T("$1"));
			fileInfo.outFileName = regex_replace(fileInfo.outFileName, wregex(__T("(.*)(\\d\\d E\\d\\d).*")), __T("$1$2"));
		}

		// And what about that data at the end?
		if (bAggressiveClean && fileInfo.outFileName.find(__T("[")) != String::npos)
		{
			do {
				pos1 = fileInfo.outFileName.find(__T("["));
				pos2 = fileInfo.outFileName.find(__T("]")) + 1;
				int remainingSize = (int(fileInfo.outFileName.size()) - pos2);
				fileInfo.outFileName = fileInfo.outFileName.substr(0, pos1) + fileInfo.outFileName.substr(pos2, remainingSize);
			} while (fileInfo.outFileName.find(__T("[")) != String::npos);
		}

		// Now for optional (parenthesis)
		if (bAggressiveClean && bCleanParenthesis && fileInfo.outFileName.find(__T("(")) != String::npos)
		{
			do {
				pos1 = fileInfo.outFileName.find(__T("("));
				pos2 = fileInfo.outFileName.find(__T(")")) + 1;
				int remainingSize = (int(fileInfo.outFileName.size()) - pos2);
				fileInfo.outFileName = fileInfo.outFileName.substr(0, pos1) + fileInfo.outFileName.substr(pos2, remainingSize);
			} while (fileInfo.outFileName.find(__T("(")) != String::npos);
		}
	}

	// Clean up end spaces if any
	while (!fileInfo.subDir.empty() && fileInfo.subDir[int(fileInfo.subDir.size()) - 1] == ' ')
		fileInfo.subDir.pop_back();
	while (!fileInfo.outFileName.empty() && fileInfo.outFileName[int(fileInfo.outFileName.size()) - 1] == ' ')
		fileInfo.outFileName.pop_back();

	if (fileInfo.subDir.empty()) 
		fileInfo.subDir = fileInfo.outFileName;
}

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
		int i = 0;
		if (bRelativeTrack && forcedAudioTrack > 0 && fileInfo.audioTracks.size() <= forcedAudioTrack)
			i = forcedAudioTrack - 1;
		else {
			for (; i < fileInfo.audioTracks.size(); i++)
			{
				if (forcedAudioTrack == fileInfo.audioTracks[i].trackNum)
				{
					fileInfo.selectedAudioTrack = i;
					break;
				}
				else if (fileInfo.audioTracks[i].language == defaultAudioLang && !bFound)
				{
					fileInfo.selectedAudioTrack = i;
					bFound = true;
				}
				else if (fileInfo.audioTracks[i].language == defaultAudioLang && bFound)
				{
					if (!bChooseTrack)
						MsgColor(__T("WARNING: Found another audio track with the same language. Defaulting to first track."), msg_warn);
					else
						bChoose = true;
				}
				else if (i == int(fileInfo.audioTracks.size()) - 1 && !bFound)
				{
					MsgColor(__T("WARNING: Could not find proper audio track! Defaulting to first track."), msg_warn);
					fileInfo.selectedAudioTrack = 0;
				}
			}
		}
	}

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

	// Select Subtitle Track
	bChoose = bFound = false;
	if (fileInfo.subtitleTracks.size() == 1)
		fileInfo.selecteSubtitleTrack = 0;
	else if (!fileInfo.subtitleTracks.size())
		fileInfo.selecteSubtitleTrack = -1;
	else
	{
		int i = 0;
		if (bRelativeTrack && forcedSubtitleTrack > 0 && fileInfo.subtitleTracks.size() <= forcedSubtitleTrack)
			i = forcedSubtitleTrack - 1;
		else {
			for (; i < fileInfo.subtitleTracks.size(); i++)
			{
				if (forcedSubtitleTrack == fileInfo.subtitleTracks[i].trackNum)
				{
					fileInfo.selecteSubtitleTrack = i;
					break;
				}
				else if (fileInfo.subtitleTracks[i].language == defaultSubtitleLang && !bFound)
				{
					fileInfo.selecteSubtitleTrack = i;
					bFound = true;
				}
				else if (fileInfo.subtitleTracks[i].language == defaultSubtitleLang && bFound)
				{
					if (!bChooseTrack)
						MsgColor(__T("WARNING: Found another subtitle track with the same language. Defaulting to first track."), msg_warn);
					else
						bChoose = true;
				}
				else if (i == int(fileInfo.subtitleTracks.size()) - 1 && !bFound)
				{
					MsgColor(__T("WARNING: Could not find proper subtitle track! Defaulting to first track."), msg_warn);
					fileInfo.selecteSubtitleTrack = 0;
				}
			}
		}

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
}

videoFile getVideoInfo(const wchar_t* filepath, int jobNum)
{
	videoFile fileInfo;

	MediaInfo MI;
	MI.Open(filepath);

	if (!MI.Count_Get(Stream_Video))
	{
		MsgColor(L"ERROR: No video track found! (" + String(filepath).substr(fileInfo.filePath.find_last_of(__T("\\")) + 1, fileInfo.filePath.length() - fileInfo.filePath.find_last_of(__T("\\")) - 5) +L")", msg_erro);
		return videoFile();
	}

	fileInfo.filePath = filepath;
	fileInfo.fileName = fileInfo.filePath.substr(fileInfo.filePath.find_last_of(__T("\\")) + 1, fileInfo.filePath.length() - fileInfo.filePath.find_last_of(__T("\\")) - 5);
	fileInfo.jobNum = jobNum;

//	wcout << __T("\r\n\r\nInform with Complete=false\r\n");
//	MI.Option(__T("Complete"));
//	wcout << MI.Inform() << endl;

	MsgColor(__T("[Job ") + to_wstring(fileInfo.jobNum) + __T("/") + to_wstring(maxJobs) + __T("] MKV Info: ") + fileInfo.fileName, msg_info);

//	wcout << "Video Count: " << MI.Count_Get(Stream_Video) << " | Audio Count: " << MI.Count_Get(Stream_Audio) << " | Subtitle Count: " << MI.Count_Get(Stream_Text) << endl;
//	wcout << "Attachments: " << MI.Get(Stream_General, 0, __T("Attachments")) << endl;

	// Video Info
	if (MI.Count_Get(Stream_Video))
	{
		trackInfo tempTrack;

		tempTrack.trackNum = stoi(MI.Get(Stream_Video, 0, L"ID"))-1;

		int resx = stoi(MI.Get(Stream_Video, 0, L"Width")),
			resy = stoi(MI.Get(Stream_Video, 0, L"Height"));
		if (resx && resy)
		{
			int cd = boost::integer::gcd(resx, resy);
			tempTrack.AR = to_wstring(float(resx) / float(resy));
			tempTrack.ARx = to_wstring(min(cd, (resx / cd)));
			tempTrack.ARy = to_wstring(min(cd, (resy / cd)));
		}
		fileInfo.videoTrack = tempTrack;

		if (MI.Get(Stream_General, 0, L"Format") == L"Matroska")
			fileInfo.videoTrack.extension = L"mkv";
		else
			fileInfo.videoTrack.extension = L"mp4";
	}

	// Audio
	for (int i = 0; i < MI.Count_Get(Stream_Audio); i++)
	{
			trackInfo tempTrack;
			tempTrack.language = Languages[MI.Get(Stream_Audio, i, L"Language")];
			if (tempTrack.language.empty())
			{
				MsgColor(__T("Warning: Unknown audio language (") + MI.Get(Stream_Audio, i, L"Language") + __T(")"), msg_warn);
				tempTrack.language = MI.Get(Stream_Audio, i, L"Language");
			}

			tempTrack.trackNum = stoi(MI.Get(Stream_Audio, i, L"ID"))-1;
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
			else
			{
				tempTrack.extension = L"m4a";
				tempTrack.bReencode = true;
				MsgColor(__T("Warning: Unknown audio format (") + MI.Get(Stream_Audio, i, L"Format") + __T("). Will re-encode regardless if chosen."), msg_warn);
			}

			fileInfo.audioTracks.push_back(tempTrack);
	}

	// Subtitles
	for (int i = 0; i < MI.Count_Get(Stream_Text); i++)
	{
		trackInfo tempTrack;
		tempTrack.language = Languages[MI.Get(Stream_Text, i, L"Language")];
		if (tempTrack.language.empty())
		{
			MsgColor(__T("Warning: Unknown subtitle language (") + MI.Get(Stream_Text, i, L"Language") + __T(")"), msg_warn);
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
		else
		{
			tempTrack.extension = L"ass";
			MsgColor(__T("Warning: Unknown subtitle format (") + MI.Get(Stream_Text, i, L"Format") + __T("). Will re-encode regardless if chosen."), msg_warn);
			break;
		}

		fileInfo.subtitleTracks.push_back(tempTrack);
	}

	// Attachments
	if (!MI.Get(Stream_General, 0, __T("Attachments")).empty())
	{
		String tmpString = MI.Get(Stream_General, 0, __T("Attachments"));
		int pos1 = 0, nextPos = 0, i=1;

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

void extractMKV(videoFile fileInfo)
{
	MsgColor(__T("[Job ") + to_wstring(fileInfo.jobNum) + __T("/") + to_wstring(maxJobs) + __T("] MKV Extract: ") + fileInfo.outFileName, msg_info);

	if (!bExtract264 && fileInfo.selectedAudioTrack < 0 && fileInfo.selecteSubtitleTrack < 0 && (!bDoAttachments || (bDoAttachments && !fileInfo.attachmentTracks.size())))
	{
		MsgColor(__T("WARNING: Nothing to extract?"), msg_warn);
		return;
	}
	
	String cmdstr = MeGUIDir + __T("tools\\mkvmerge\\mkvextract.exe \"") + fileInfo.filePath + __T("\" ");
	
	if (bExtract264)
		cmdstr += __T("tracks ") + to_wstring(fileInfo.videoTrack.trackNum) + __T(":\"") + WorkDir + fileInfo.outFileName + __T(".264\" ");

	if (fileInfo.selectedAudioTrack >= 0)
		cmdstr += __T("tracks ") + to_wstring(fileInfo.audioTracks[fileInfo.selectedAudioTrack].trackNum) + __T(":\"") + WorkDir + fileInfo.outFileName + __T(".") + fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension + __T("\" ");

	if (fileInfo.selecteSubtitleTrack >= 0)
		cmdstr += __T("tracks ") + to_wstring(fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].trackNum) + __T(":\"") + WorkDir + fileInfo.outFileName + __T(".") + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension + __T("\" ");

	if (bDoAttachments && fileInfo.attachmentTracks.size())
	{
		for each (trackInfo atch in fileInfo.attachmentTracks)
			cmdstr += __T("attachments ") + to_wstring(atch.trackNum) + __T(":\"") + WorkDir + atch.filename + __T("\" ");
	}

	_wsystem(cmdstr.data());
}

void extractMP4(videoFile fileInfo)
{
	MsgColor(__T("[Job ") + to_wstring(fileInfo.jobNum) + __T("/") + to_wstring(maxJobs) + __T("] MP4 Extract: ") + fileInfo.outFileName, msg_info);

	if (!bExtract264 && fileInfo.selectedAudioTrack < 0 && fileInfo.selecteSubtitleTrack < 0 && (!bDoAttachments || (bDoAttachments && !fileInfo.attachmentTracks.size())))
	{
		MsgColor(__T("WARNING: Nothing to extract?"), msg_warn);
		return;
	}

	String cmdstr = MeGUIDir + __T("tools\\mp4box\\mp4box.exe \"") + fileInfo.filePath + __T("\" ");

	if (bExtract264)
		cmdstr += __T("-raw ") + to_wstring(fileInfo.videoTrack.trackNum + 1) + __T(":output=\"") + WorkDir + fileInfo.outFileName + __T(".264\" ");

	if (fileInfo.selectedAudioTrack >= 0)
		cmdstr += __T("-raw ") + to_wstring(fileInfo.audioTracks[fileInfo.selectedAudioTrack].trackNum + 1) + __T(":output=\"") + WorkDir + fileInfo.outFileName + __T(".") + fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension + __T("\" ");

	if (fileInfo.selecteSubtitleTrack >= 0)
		cmdstr += __T("-raw ") + to_wstring(fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].trackNum + 1) + __T(":output=\"") + WorkDir + fileInfo.outFileName + __T(".") + fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension + __T("\" ");

	_wsystem(cmdstr.data());
}

void createMeGUIJobs(videoFile fileInfo)
{
	String lJobTag = __T("\t\t<string>");
	String rJobTag = __T("</string>\n");
	String tempList = __T("");

	MsgColor(__T("[Job ") + to_wstring(fileInfo.jobNum) + __T("/") + to_wstring(maxJobs) + __T("] Creating MeGUI Jobs: ") + fileInfo.outFileName, msg_info);

	// AVS Script
	wofstream myAVSFile(String(WorkDir + fileInfo.outFileName + __T(".avs")));
	if (myAVSFile.is_open())
	{
		try {
			if (!bExtract264)
				myAVSFile << wformat(AVSTemplate) % fileInfo.filePath;
			else
				myAVSFile << wformat(AVSTemplate_264) % fileInfo.outFileName % WorkDir;

			if (fileInfo.selecteSubtitleTrack != -1 && (fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension == __T("ass") || fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension == __T("srt")))
				myAVSFile << wformat(AVSTemplate_Subs) % fileInfo.outFileName % WorkDir % fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension;
			else if (fileInfo.selecteSubtitleTrack != -1)
				myAVSFile << wformat(AVSTemplate_Sups) % fileInfo.outFileName % WorkDir % fileInfo.subtitleTracks[fileInfo.selecteSubtitleTrack].extension;
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
	
	// 264 Job Script
	wofstream myVideoJob(String(MeGUIDir + __T("jobs\\job") + to_wstring(jobNumber) + __T(" - ") + fileInfo.outFileName + __T(" - 264.xml")));
	if (myVideoJob.is_open())
	{
		try {
			myVideoJob << wformat(videoJobTemplate) % fileInfo.outFileName % WorkDir % WorkDir % fileInfo.videoTrack.AR % fileInfo.videoTrack.ARx % fileInfo.videoTrack.ARy % to_wstring(jobNumber) % to_wstring(jobNumber + (fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? 2 : 1));
			wcout << "Created 264 Job." << endl;
			tempList += lJobTag + __T("job") + to_wstring(jobNumber) + __T(" - ") + fileInfo.outFileName + __T(" - 264") + rJobTag;
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

	// Audio Job Script
	if (fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode)
	{
		wofstream myAudioJob(String(MeGUIDir + __T("jobs\\job") + to_wstring(jobNumber) + __T(" - ") + fileInfo.outFileName + __T(" - AC3.xml")));
		if (myAudioJob.is_open())
		{
			try {
				myAudioJob << wformat(audioJobTemplate) % fileInfo.outFileName % WorkDir % fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension % to_wstring(jobNumber);
				wcout << "Created AC3 Job." << endl;
				tempList += lJobTag + __T("job") + to_wstring(jobNumber) + __T(" - ") + fileInfo.outFileName + __T(" - AC3") + rJobTag;
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
			MsgColor(__T("ERROR: Unable to open AC3 XML file!"), msg_erro);
			return;
		}
	}

	// Mux Job Script
	wofstream myMuxJob(String(MeGUIDir + __T("jobs\\job") + to_wstring(jobNumber)  + __T(" - ") + fileInfo.outFileName + __T(" - MKV.xml")));
	if (myMuxJob.is_open())
	{
		try {
			String outDir = OutputDir;
			if (bCreateSubDir && !fileInfo.subDir.empty())
				outDir += fileInfo.subDir + __T("\\");

			String framerate = __T(" xsi:nil=\"true\" />");
			if (!fileInfo.videoTrack.framerate.empty())
				framerate = L">" + fileInfo.videoTrack.framerate + __T("</Framerate>");
			if (!bExtract264)
				myMuxJob << wformat(muxJobTemplate) % fileInfo.outFileName % WorkDir % WorkDir % outDir % (fileInfo.selectedAudioTrack >= 0 ? (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension : L"ac3") : L"m4a") % framerate % to_wstring(jobNumber) % to_wstring(jobNumber - (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? 2 : 1)) % fileInfo.filePath;
			else
				myMuxJob << wformat(muxJobTemplate_264) % fileInfo.outFileName % WorkDir % WorkDir % outDir % (fileInfo.selectedAudioTrack >= 0 ? (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? fileInfo.audioTracks[fileInfo.selectedAudioTrack].extension : L"ac3") : L"m4a") % framerate % to_wstring(jobNumber) % to_wstring(jobNumber - (!fileInfo.audioTracks[fileInfo.selectedAudioTrack].bReencode ? 2 : 1));

			wcout << L"Created Mux Job." << endl;
			tempList += lJobTag + __T("job") + to_wstring(jobNumber) + __T(" - ") + fileInfo.outFileName + __T(" - MKV") + rJobTag;
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
		MsgColor(__T("ERROR: Unable to open MKV XML file!"), msg_erro);
		return;
	}

	// Add Job List
	wifstream myJobListRead(String(MeGUIDir + __T("joblists.xml")));
	String curLine;
	vector<String> jobListRAW;
	if (myJobListRead.is_open())
	{
		
		while (getline(myJobListRead, curLine))
			jobListRAW.push_back(curLine);

		myJobListRead.close();
	}
	else 
	{
		MsgColor(__T("ERROR: Unable to open joblists.xml to read!"), msg_erro);
		return;
	}

	wofstream myJobListWrite(String(MeGUIDir + __T("joblists.xml")));
	if (myJobListWrite.is_open())
	{
		try {
			if (jobListRAW.size() == 0)
				myJobListWrite << wformat(jobListTemplate) % tempList;
			else
			{
				int x;
				for (x = 0; x <= int(jobListRAW.size()) - 1; x++)
				{
					if (jobListRAW[x].find(fileInfo.outFileName) != String::npos)
					{
						jobListRAW.erase(jobListRAW.begin() + x);
						x--;
						continue;
					}
					
					else if (jobListRAW[x].find(L"<mainJobList />") != String::npos)
						jobListRAW[x] = __T("<mainJobList>\n") + tempList + __T("</mainJobList>");
					else if (jobListRAW[x].find(L"</mainJobList>") != String::npos)
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
		MsgColor(__T("ERROR: Unable to open joblist.xml!"), msg_erro);
		return;
	}
}

void clearMeGUIJobs()
{
	wifstream myJobListRead(String(MeGUIDir + __T("joblists.xml")));
	String curLine;
	vector<String> jobListRAW;
	if (myJobListRead.is_open())
	{
		while (getline(myJobListRead, curLine))
			jobListRAW.push_back(curLine);

		myJobListRead.close();
	}
	else
	{
		MsgColor(__T("ERROR: Unable to open joblist.xml!"), msg_erro);
		return;
	}

	MsgColor(__T("Clearing MeGUI Jobs"), msg_info);

	wofstream myJobListWrite(String(MeGUIDir + __T("joblists.xml")));
	if (myJobListWrite.is_open())
	{
		try {
			int x;
			for (x = 0; x <= int(jobListRAW.size()) - 1; x++)
			{
				if ((bClearMeGUIJobs && jobListRAW[x].find(L"String") != String::npos))
				{
					if (bClearMeGUIJobs && jobListRAW[x].find(L"String") != String::npos)
					{

						String file = jobListRAW[x].substr(jobListRAW[x].find(L">") + 1, jobListRAW[x].find(L"</") - (jobListRAW[x].find(L">") + 1));
						if (_wremove(String(MeGUIDir + __T("\\jobs\\") + file + __T(".xml")).c_str()) == 0)
							wcout << __T("Removed: MeGUI\\jobs\\") + file + __T(".xml\n");
					}
					jobListRAW.erase(jobListRAW.begin() + x);
					x--;
					continue;
				}
				else if (bClearMeGUIJobs && jobListRAW[x].find(L"<mainJobList>") != String::npos)
				{
					jobListRAW[x] = regex_replace(jobListRAW[x], wregex(__T("mainJobList")), __T("mainJobList /"));
				}
				else if (bClearMeGUIJobs && jobListRAW[x].find(L"</mainJobList>") != String::npos)
				{
					jobListRAW.erase(jobListRAW.begin() + x);
					x--;
					continue;
				}
				
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
		MsgColor(__T("ERROR: Unable to open joblist.xml!"), msg_erro);
		return;
	}
}

int processOptions(int ac, wchar_t* av[])
{
	try {
		String config_file;

		string _config_file, _MeGUIDir, _WorkDir, _OutputDir, _defaultAudioLang, _defaultSubtitleLang, _customEpisodeRegex;

		// Declare a group of options that will be allowed only on command line
		po::options_description basic("Base Options");
		basic.add_options()
			("help,h", "Produce help message")
			("config,c", po::value<string>(&_config_file)->default_value("MeGUIHelper.cfg"), "Name of configuration file.")
			;

		// Declare a group of options that will be allowed both on command line and in config file
		po::options_description directories("Directories");
		directories.add_options()
			("MeGUI-Dir", po::value< string >(&_MeGUIDir), "Path to MeGUI (Required)")
			("Work-Dir,w", po::value< string >(&_WorkDir), "Temporary raw file location\n(Default: MeGUI Helper's location)")
			("Output-Dir,o", po::value< string >(&_OutputDir), "Output of encoded files\n(Default: MeGUI Helper's location)")
			;

		po::options_description operations("Operation Options");
		operations.add_options()
			("bJobFilesOnly,J", po::value<bool>(&bJobFilesOnly)->default_value(false), "Create job scripts only. (DOES NOT EXTRACT)")
			("bClearMeGUIJobs,j", po::value<bool>(&bClearMeGUIJobs)->default_value(false), "Clean all old jobs (MeGUI must be closed!).")
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

		ifstream ifs(_config_file.c_str());
		if (!ifs)
		{
			MsgColor(__T("ERROR: Cannot open config file: ") + config_file, msg_erro);
			return errno;
		}
		else
		{
			store(parse_config_file(ifs, config_file_options), vm);
			notify(vm);
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
		
		if ( !(cPath = _wgetcwd(NULL,0)) )
			return errno;

		if (MeGUIDir.empty())
		{
			MsgColor(__T("ERROR: Please fix your config file to include a working path to mkvtoolnix and MeGUI!"), msg_erro);
			return errno;
		}
		else if (MeGUIDir[int(MeGUIDir.size()) - 1] != '//')
			MeGUIDir += __T("\\");

		if (!WorkDir.empty())
		{
			if (WorkDir[int(WorkDir.size()) - 1] != '//')
				WorkDir += __T("\\");
		}
		else
			WorkDir = String(cPath) + __T("//");

		if (!OutputDir.empty())
		{
			if (OutputDir[int(OutputDir.size()) - 1] != '//')
				OutputDir += __T("\\");
		}
		else
			OutputDir = String(cPath) + __T("//");
			
	}
	catch (std::exception e)
	{
		wcout << e.what() << "\n";
		return errno;
	}
	return 0;
}

void initLang()
{
	Languages[L"en"] = L"eng";
	Languages[L"ja"] = L"jpn";
	Languages[L"zh"] = L"chn";
	Languages[L"gr"] = L"ger";
	Languages[L"ru"] = L"rus";
	Languages[L"ko"] = L"kor";
}

int wmain(int argc, wchar_t *argv[])
{
	videoFile tempVideo;
	String tempArg;
	vector<String> args;
	vector<String> vidPaths;

	for (size_t i = 0; i < argc; ++i)
	{
		tempArg = argv[i];
		if (tempArg.length() > 4 && (tempArg.substr(tempArg.length() - 4, 4) == L".mkv" || tempArg.substr(tempArg.length() - 4, 4) == L".mp4"))
			continue;
		args.push_back(tempArg);
	}

	wchar_t ** av = new wchar_t*[args.size()];
	for (size_t i = 0; i < args.size(); i++)
	{
		av[i] = new wchar_t[args[i].size() + 1];
		wcscpy(av[i], args[i].c_str());
	}

	if (processOptions(args.size(), av))
	{
		MsgColor(__T("Error loading options/config."), msg_erro);
		return errno;
	}

	for (size_t i = 0; i < args.size(); i++)
	{
		delete [] av[i];
	}
	delete[] av;

	initLang();

	for (int i = 1; i < argc; ++i)
	{
		tempArg = argv[i];
		if (tempArg.length() > 4 && (tempArg.substr(tempArg.length() - 4, 4) == L".mkv" || tempArg.substr(tempArg.length() - 4, 4) == L".mp4"))
		{
			vidPaths.push_back(tempArg);
			maxJobs++;
		}
	}

	for (int i = 0; i < vidPaths.size(); i++)
	{
		tempVideo = getVideoInfo(vidPaths[i].data(), i + 1);
		
		if (tempVideo.fileName == L"")
			continue;
		else if (tempVideo.audioTracks.empty())
		{
			MsgColor(__T("WARNING: No audio detected!"), msg_warn);
			continue;
		}

		videoList.push_back(tempVideo);
	}

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

		if (bDoAttachments && bInstallFonts && !bJobFilesOnly)
		{
			for (map<String, bool>::iterator it = attchmentList.begin(); it != attchmentList.end(); ++it)
			{
				tempArg = WorkDir + it->first;
				AddFontResource((LPCWSTR)tempArg.c_str());
			}

			MsgColor(__T("Please make sure to manually install any fonts that were extracted!"), msg_whit);
		}

		wcout << "Run MeGUI? (Y) ";
		getline(wcin, tempArg);
		if (tempArg == L"y" || tempArg == L"Y" || tempArg == L"")
			_wsystem(String(L"start " + MeGUIDir + L"MeGUI.exe").data());
	}
	else
		wcout << ": No MKV files in arguement!" << endl << "Use: MeGUIHelper.exe [filename] ..." << endl;

	return 0;
}