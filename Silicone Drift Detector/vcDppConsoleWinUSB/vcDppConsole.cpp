/** vcDppConsole.cpp */

// vcDppConsole.cpp : Defines the entry point for the console application.
#include "stdafx.h"
using namespace std; 
#include "ConsoleHelper.h"
#include "stringex.h"

CConsoleHelper chdpp;					// DPP communications functions
bool bRunSpectrumTest = false;			// run spectrum test
bool bRunConfigurationTest = false;		// run configuration test
bool bHaveStatusResponse = false;		// have status response
bool bHaveConfigFromHW = false;			// have configuration from hardware

// connect to default dpp
//		CConsoleHelper::WinUSB_Connect_Default_DPP	// WinUSB connect to default DPP
void ConnectToDefaultDPP()
{
	cout << endl;
	cout << "Running DPP WinUSB tests from console..." << endl;
	cout << endl;
	cout << "\tConnecting to default WinUSB device..." << endl;
	if (chdpp.WinUSB_Connect_Default_DPP()) {
		cout << "\t\tWinUSB DPP device connected." << endl;
		cout << "\t\tWinUSB DPP devices present: "  << chdpp.WinUSB_NumDevices << endl;
	} else {
		cout << "\t\tWinUSB DPP device not connected." << endl;
		cout << "\t\tNo WinUSB DPP device present." << endl;
	}
}

// Get DPP Status
//		CConsoleHelper::WinUSB_isConnected						// check if DPP is connected
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_SEND_STATUS)	// request status
//		CConsoleHelper::WinUSB_ReceiveData()					// parse the status
//		CConsoleHelper::DppStatusString							// display status string
void GetDppStatus()
{
	if (chdpp.WinUSB_isConnected) { // send and receive status
		cout << endl;
		cout << "\tRequesting Status..." << endl;
		if (chdpp.WinUSB_SendCommand(XMTPT_SEND_STATUS)) {	// request status
			cout << "\t\tStatus sent." << endl;
			cout << "\t\tReceiving status..." << endl;
			if (chdpp.WinUSB_ReceiveData()) {
				cout << "\t\t\tStatus received..." << endl;
				cout << chdpp.DppStatusString << endl;
				bRunSpectrumTest = true;
				bHaveStatusResponse = true;
				bRunConfigurationTest = true;
			} else {
				cout << "\t\tError receiving status." << endl;
			}
		} else {
			cout << "\t\tError sending status." << endl;
		}
	}
}

// Read Full DPP Configuration From Hardware			// request status before sending/receiving configurations
//		CONFIG_OPTIONS									// holds configuration command options
//		CConsoleHelper::CreateConfigOptions				// creates configuration options from last status read
//      CConsoleHelper::ClearConfigReadFormatFlags();	// clear configuration format flags, for cfg readback
//      CConsoleHelper::CfgReadBack = true;				// requesting general readback format
//		CConsoleHelper::WinUSB_SendCommand_Config		// send command with options
//		CConsoleHelper::WinUSB_ReceiveData()			// parse the configuration
//		CConsoleHelper::HwCfgReady						// config is ready
void ReadDppConfigurationFromHardware(bool bDisplayCfg)
{
	CONFIG_OPTIONS CfgOptions;
	if (bHaveStatusResponse && bRunConfigurationTest) {
		//test configuration functions
		// Set options for XMTPT_FULL_READ_CONFIG_PACKET
		chdpp.CreateConfigOptions(&CfgOptions, "", chdpp.DP5Stat, false);
		cout << endl;
		cout << "\tRequesting Full Configuration..." << endl;
		chdpp.ClearConfigReadFormatFlags();	// clear all flags, set flags only for specific readback properties
		//chdpp.DisplayCfg = false;	// DisplayCfg format overrides general readback format
		chdpp.CfgReadBack = true;	// requesting general readback format
		if (chdpp.WinUSB_SendCommand_Config(XMTPT_FULL_READ_CONFIG_PACKET, CfgOptions)) {	// request full configuration
			if (chdpp.WinUSB_ReceiveData()) {
				if (chdpp.HwCfgReady) {		// config is ready
					bHaveConfigFromHW = true;
					if (bDisplayCfg) {
						cout << "\t\t\tConfiguration Length: " << (unsigned int)chdpp.HwCfgDP5.length() << endl;
						cout << "\t================================================================" << endl;
						cout << chdpp.HwCfgDP5 << endl;
						cout << "\t================================================================" << endl;
						cout << "\t\t\tScroll up to see configuration settings." << endl;
						cout << "\t================================================================" << endl;
					} else {
						cout << "\t\tFull configuration received." << endl;
					}
				}
			}
		}
	}
}

// Display Preset Settings
//		CConsoleHelper::strPresetCmd	// preset mode
//		CConsoleHelper::strPresetVal	// preset setting
void DisplayPresets()
{
	if (bHaveConfigFromHW) {
		cout << "\t\t\tPreset Mode: " << chdpp.strPresetCmd << endl;
		cout << "\t\t\tPreset Settings: " << chdpp.strPresetVal << endl;
	}
}

// Display Preset Settings
//		CONFIG_OPTIONS								// holds configuration command options
//		CConsoleHelper::CreateConfigOptions			// creates configuration options from last status read
//		CConsoleHelper::HwCfgDP5Out					// preset setting
//		CConsoleHelper::WinUSB_SendCommand_Config	// send command with options
void SendPresetAcquisitionTime(string strPRET)
{
	CONFIG_OPTIONS CfgOptions;
	cout << "\tSetting Preset Acquisition Time..." << strPRET << endl;
	chdpp.CreateConfigOptions(&CfgOptions, "", chdpp.DP5Stat, false);
	CfgOptions.HwCfgDP5Out = strPRET;
	// send PresetAcquisitionTime string, bypass any filters, read back the mode and settings
	if (chdpp.WinUSB_SendCommand_Config(XMTPT_SEND_CONFIG_PACKET_EX, CfgOptions)) {
		ReadDppConfigurationFromHardware(false);	// read setting back
		DisplayPresets();							// display new presets
	} else {
		cout << "\t\tPreset Acquisition Time NOT SET" << strPRET << endl;
	}
}

// Acquire Spectrum
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_DISABLE_MCA_MCS)		//disable for data/status clear
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_SEND_CLEAR_SPECTRUM_STATUS)  //clear spectrum/status
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_ENABLE_MCA_MCS);		// enabling MCA for spectrum acquisition
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_SEND_SPECTRUM_STATUS)) // request spectrum+status
//		CConsoleHelper::WinUSB_ReceiveData()							// process spectrum and data
//		CConsoleHelper::ConsoleGraph()	(low resolution display)		// graph data on console with status
//		CConsoleHelper::WinUSB_SendCommand(XMTPT_DISABLE_MCA_MCS)		// disable mca after acquisition
void AcquireSpectrum()
{
	int MaxMCA = 11;
	bool bDisableMCA;

	//bRunSpectrumTest = false;		// disable test
	if (bRunSpectrumTest) {
		cout << "\tRunning spectrum test..." << endl;
		cout << "\t\tDisabling MCA for spectrum data/status clear." << endl;
		chdpp.WinUSB_SendCommand(XMTPT_DISABLE_MCA_MCS);
		Sleep(1000);
		cout << "\t\tClearing spectrum data/status." << endl;
		chdpp.WinUSB_SendCommand(XMTPT_SEND_CLEAR_SPECTRUM_STATUS);
		Sleep(1000);
		cout << "\t\tEnabling MCA for spectrum data acquisition with status ." << endl;
		chdpp.WinUSB_SendCommand(XMTPT_ENABLE_MCA_MCS);
		Sleep(1000);
		for(int idxSpectrum=0;idxSpectrum<MaxMCA;idxSpectrum++) {
			//cout << "\t\tAcquiring spectrum data set " << (idxSpectrum+1) << " of " << MaxMCA << endl;
			if (chdpp.WinUSB_SendCommand(XMTPT_SEND_SPECTRUM_STATUS)) {	// request spectrum+status
				if (chdpp.WinUSB_ReceiveData()) {
					bDisableMCA = true;				// we are aquiring data, disable mca when done
					system("cls");
					chdpp.ConsoleGraph(chdpp.DP5Proto.SPECTRUM.DATA,chdpp.DP5Proto.SPECTRUM.CHANNELS,true,chdpp.DppStatusString);
					Sleep(2000);
				}
			} else {
				cout << "\t\tProblem acquiring spectrum." << endl;
				break;
			}
		}
		if (bDisableMCA) {
			//system("Pause");
			//cout << "\t\tSpectrum acquisition with status done. Disabling MCA." << endl;
			chdpp.WinUSB_SendCommand(XMTPT_DISABLE_MCA_MCS);
			Sleep(1000);
		}
	}
}

// Read Configuration File
//		CConsoleHelper::SndCmd.GetDP5CfgStr("PX5_Console_Test.txt");
void ReadConfigFile()
{
	std::string strCfg;
	strCfg = chdpp.SndCmd.AsciiCmdUtil.GetDP5CfgStr("PX5_Console_Test.txt");
	cout << "\t\t\tConfiguration Length: " << (unsigned int)strCfg.length() << endl;
	cout << "\t================================================================" << endl;
	cout << strCfg << endl;
	cout << "\t================================================================" << endl;
}

//Following is an example of loading a configuration from file 
//then sending the configuration to the DPP device.
//	SendConfigFileToDpp("NaI_detector_cfg.txt");    // calls SendCommandString
//	AcquireSpectrum();
//
bool SendCommandString(string strCMD) {
    CONFIG_OPTIONS CfgOptions;
    chdpp.CreateConfigOptions(&CfgOptions, "", chdpp.DP5Stat, false);
    CfgOptions.HwCfgDP5Out = strCMD;
    // send ASCII command string, bypass any filters, read back the mode and settings
    if (chdpp.WinUSB_SendCommand_Config(XMTPT_SEND_CONFIG_PACKET_EX, CfgOptions)) {
        // command sent
    } else {
        cout << "\t\tASCII Command String NOT SENT" << strCMD << endl;
        return false;
    }
    return true;
}

std::string ShortenCfgCmds(std::string strCfgIn) {
    std::string strCfg("");
	strCfg = strCfgIn;
	long lCfgLen=0;						//ASCII Configuration Command String Length
    lCfgLen = (long)strCfg.length();
	if (lCfgLen > 0) {		
        strCfg = chdpp.SndCmd.AsciiCmdUtil.ReplaceCmdText(strCfg, "US;", ";");
        strCfg = chdpp.SndCmd.AsciiCmdUtil.ReplaceCmdText(strCfg, "OFF;", "OF;");
        strCfg = chdpp.SndCmd.AsciiCmdUtil.ReplaceCmdText(strCfg, "RISING;", "RI;");
        strCfg = chdpp.SndCmd.AsciiCmdUtil.ReplaceCmdText(strCfg, "FALLING;", "FA;");
	}
	return strCfg;
}

// run GetDppStatus(); first to get PC5_PRESENT, DppType
// Includes Configuration Oversize Fix 20141224
bool SendConfigFileToDpp(string strFilename){
    std::string strCfg;
	long lCfgLen=0;						//ASCII Configuration Command String Length
    bool bCommandSent=false;
	bool isPC5Present=false;
	int DppType=0;
	int idxSplitCfg=0;					//Configuration split position, only if necessary
	bool bSplitCfg=false;				//Configuration split flag
	std::string strSplitCfg("");		//Configuration split string second buffer
	bool isDP5_RevDxGains;
	unsigned char DPP_ECO;

	isPC5Present = chdpp.DP5Stat.m_DP5_Status.PC5_PRESENT;
	DppType = chdpp.DP5Stat.m_DP5_Status.DEVICE_ID;
	isDP5_RevDxGains = chdpp.DP5Stat.m_DP5_Status.isDP5_RevDxGains;
	DPP_ECO = chdpp.DP5Stat.m_DP5_Status.DPP_ECO;

    strCfg = chdpp.SndCmd.AsciiCmdUtil.GetDP5CfgStr(strFilename);
	strCfg = chdpp.SndCmd.AsciiCmdUtil.RemoveCmdByDeviceType(strCfg,isPC5Present,DppType,isDP5_RevDxGains,DPP_ECO);
    lCfgLen = (long)strCfg.length();
    if ((lCfgLen > 0) && (lCfgLen <= 512)) {		// command length ok
        cout << "\t\t\tConfiguration Length: " << lCfgLen << endl;
    } else if (lCfgLen > 512) {	// configuration too large, needs fix
		cout << "\t\t\tConfiguration Length (Will Shorten): " << lCfgLen << endl;
		strCfg = ShortenCfgCmds(strCfg);
		lCfgLen = (long)strCfg.length();
		if (lCfgLen > 512) {	// configuration still too large, split config
			cout << "\t\t\tConfiguration Length (Will Split): " << lCfgLen << endl;
            bSplitCfg = true;
            idxSplitCfg = chdpp.SndCmd.AsciiCmdUtil.GetCmdChunk(strCfg);
			cout << "\t\t\tConfiguration Split at: " << idxSplitCfg << endl;
            strSplitCfg = strCfg.substr(idxSplitCfg);
			strCfg = strCfg.substr(0, idxSplitCfg);
		}
    } else {
        cout << "\t\t\tConfiguration Length Error: " << lCfgLen << endl;
        return false;
    }
	bCommandSent = SendCommandString(strCfg);
	if (bSplitCfg) {
		// Sleep(40);			// may need delay here
		bCommandSent = SendCommandString(strSplitCfg);
	}
    return bCommandSent;
}

// Close Connection
//		CConsoleHelper::WinUSB_isConnected			// WinUSB DPP connection indicator
//		CConsoleHelper::WinUSB_Close_Connection()	// close connection
void CloseConnection()
{
	if (chdpp.WinUSB_isConnected) { // send and receive status
		cout << endl;
		cout << "\tClosing connection to default WinUSB device..." << endl;
		chdpp.WinUSB_Close_Connection();
		cout << "\t\tDPP device connection closed." << endl;
	}
}

// Helper functions for saving spectrum files
void SaveSpectrumConfig()
{
	string strSpectrumConfig;
	chdpp.Dp5CmdList = chdpp.MakeDp5CmdList();	// ascii text command list for adding comments
	strSpectrumConfig = chdpp.CreateSpectrumConfig(chdpp.HwCfgDP5);	// append configuration comments
	chdpp.sfInfo.strSpectrumConfig = strSpectrumConfig;
}

// Saving spectrum file
void SaveSpectrumFile()
{
	string strSpectrum;											// holds final spectrum file
	chdpp.sfInfo.strSpectrumStatus = chdpp.DppStatusString;		// save last status after acquisition
	chdpp.sfInfo.m_iNumChan = chdpp.mcaCH;						// number channels in spectrum
	chdpp.sfInfo.SerialNumber = chdpp.DP5Stat.m_DP5_Status.SerialNumber;	// dpp serial number
	chdpp.sfInfo.strDescription = "Amptek Spectrum File";					// description
    chdpp.sfInfo.strTag = "TestTag";										// tag
	// create spectrum file, save file to string
    strSpectrum = chdpp.CreateMCAData(chdpp.DP5Proto.SPECTRUM.DATA,chdpp.sfInfo,chdpp.DP5Stat.m_DP5_Status);
	chdpp.SaveSpectrumStringToFile(strSpectrum);	// save spectrum file string to file
}

/** _tmain 
*/
int _tmain(int argc, _TCHAR* argv[])
{

	system("cls");
	ConnectToDefaultDPP();
	system("Pause");

	if(!chdpp.WinUSB_isConnected) { return 1; }

	system("cls");
	chdpp.DP5Stat.m_DP5_Status.SerialNumber = 0;
	GetDppStatus();
	system("Pause");

	if (chdpp.DP5Stat.m_DP5_Status.SerialNumber == 0) { return 1; }

	//////	system("cls");
	//////	SendConfigFileToDpp("PX5_Console_Test.txt");    // calls SendCommandString
	//////	system("Pause");

	system("cls");
	ReadDppConfigurationFromHardware(true);
	system("Pause");

	system("cls");
	DisplayPresets();
	system("Pause");

	system("cls");
	SendPresetAcquisitionTime("PRET=20;");
	SaveSpectrumConfig();
	system("Pause");

	system("cls");
	AcquireSpectrum();
	SaveSpectrumFile();
	system("Pause");

	system("cls");
	SendPresetAcquisitionTime("PRET=OFF;");
	system("Pause");

	system("cls");
	ReadConfigFile();
	system("Pause");

	system("cls");
	CloseConnection();
	system("Pause");

	return 0;
}





















