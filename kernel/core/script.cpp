/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


// Implementation der Script-Klasse

#include "script.hpp"
#include "../kernel.hpp"
#include "utils/tinyxml2.h"

#include <algorithm>


/////////////////////////////////////////////////
/// \brief Default constructor
/////////////////////////////////////////////////
Script::Script() : FileSystem(), _localDef(true)
{
    sScriptFileName = "";
    sIncludeFileName = "";
    sHelpID = "";
    sInstallID = "";
    bValidScript = false;
    bScriptOpen = false;
    bReadFromInclude = false;
    bAutoStart = false;
    bLastScriptCommand = false;
    bBlockComment = false;
    bInstallProcedures = false;
    bENABLE_FULL_LOGGING = false;
    bDISABLE_SCREEN_OUTPUT = false;
    bIsInstallingProcedure = false;
    nCurrentPackage = 0;
    nLine = 0;
    nIncludeLine = 0;
    nIncludeType = 0;
}


/////////////////////////////////////////////////
/// \brief Specialized constructor taking a
/// script file name as argument.
///
/// \param _sScriptFileName const string&
///
/////////////////////////////////////////////////
Script::Script(const string& _sScriptFileName) : Script()
{
    sScriptFileName = _sScriptFileName;
}


/////////////////////////////////////////////////
/// \brief Destructor
/////////////////////////////////////////////////
Script::~Script()
{
    if (bScriptOpen)
    {
        Script::close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function removes line
/// comments from a script line.
///
/// \param sLine const string&
/// \return string
///
/// It does not remove documentation lines, if an
/// install section is currently executed and
/// we're inside of a procedure.
/////////////////////////////////////////////////
string Script::stripLineComments(const string& sLine) const
{
    // Comment signs are the only chars? -> return an empty string
    if (sLine == "##")
        return "";

    size_t nQuotes = 0;

    // Go through the current line and search for line comments,
    // which are not masked by quotation marks
    for (size_t i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
            nQuotes++;

        // Found a documentation string
        if (sLine.substr(i, 3) == "##!" && bInstallProcedures && bIsInstallingProcedure && !(nQuotes % 2))
            return sLine;

        // Found a line comment
        if (sLine.substr(i, 2) == "##" && !(nQuotes % 2))
            return sLine.substr(0,i);
    }
    return sLine;
}


/////////////////////////////////////////////////
/// \brief This member function removes block
/// comments from a script line.
///
/// \param sLine const string&
/// \return string
///
/// It does not remove documentation lines, if an
/// install section is currently executed and
/// we're inside of a procedure.
/////////////////////////////////////////////////
string Script::stripBlockComments(const string& sLine)
{
    // Comment signs are the only chars? -> return an empty string
    // and activate the block comment flag
    if (sLine == "#*")
    {
        bBlockComment = true;
        return "";
    }

    size_t nQuotes = 0;
    string sReturn = sLine;

    // Go through the current line and search for block comments,
    // which are not masked by quotation marks
    for (size_t i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == '"' && (!i || sReturn[i-1] != '\\'))
            nQuotes++;

        // Found a documentation string
        if (sLine.substr(i, 3) == "#*!" && bInstallProcedures && bIsInstallingProcedure && !(nQuotes % 2))
            return sLine;

        // Found a block comment
        if (sReturn.substr(i,2) == "#*" && !(nQuotes % 2))
        {
            // Search for the end
            for (size_t j = i+2; j < sReturn.length(); j++)
            {
                if (sReturn.substr(j,2) == "*#")
                {
                    sReturn.erase(i,j+2-i);
                    break;
                }
                if (j+1 == sReturn.length())
                {
                    sReturn.erase(i);
                    bBlockComment = true;
                    break;
                }
            }
        }
    }
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This member function opens a script
/// with an already defined script file name.
///
/// \return void
///
/////////////////////////////////////////////////
void Script::openScript()
{
    // Close an already opened script
    if (fScript.is_open())
        fScript.close();

    // Open the script, if the script file name exists
    if (sScriptFileName.length())
    {
        fScript.open(sScriptFileName.c_str(), ios_base::in);
        bScriptOpen = true;

        // Ensure that the script exists and is a valid file
        if (fScript.fail())
        {
            string sErrorToken = sScriptFileName;
            close();
            sScriptFileName = "";
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, "", SyntaxError::invalid_position, sErrorToken);
        }

        // Set the defaults
        bReadFromInclude = false;
        bValidScript = true;
        bAutoStart = false;
        bENABLE_FULL_LOGGING = false;
        bDISABLE_SCREEN_OUTPUT = false;
        bIsInstallingProcedure = false;
        sHelpID = "";
        sInstallID = "";
        nLine = 0;
        nIncludeLine = 0;
        _localDef.reset();
    }
    return;
}


/////////////////////////////////////////////////
/// \brief This member function opens the script
/// with the passed file name.
///
/// \param _sScriptFileName string&
/// \return void
///
/////////////////////////////////////////////////
void Script::openScript(string& _sScriptFileName)
{
    setScriptFileName(_sScriptFileName);
    openScript();
    return;
}


/////////////////////////////////////////////////
/// \brief This member function closes an opened
/// script.
///
/// \return void
///
/////////////////////////////////////////////////
void Script::close()
{
    // If a script is open
    if (bScriptOpen)
    {
        // Close the file streams
        fScript.close();
        if (fInclude.is_open())
            fInclude.close();

        // If the file stream of the installation log
        // is still open, add a installation failed message
        // to it and close it afterwards
        if (fLogFile.is_open())
        {
            fLogFile << "--- INSTALLATION FAILED ---" << endl << endl << endl;
            fLogFile.close();
        }

        // If this is a chained installation (a.k.a. installing
        // multiple packages), then we don't want to reset the
        // flags but open the next script instead
        if (vInstallPackages.size() > nCurrentPackage+1)
        {
            bLastScriptCommand = false;
            nCurrentPackage++;

            // Open the file and hope, it
            // actually exist
            openScript(vInstallPackages[nCurrentPackage]);

            return;
        }

        if (vInstallPackages.size())
            sScriptFileName = vInstallPackages[0];

        // This was the last package
        vInstallPackages.clear();
        nCurrentPackage = 0;

        // Reset the flags
        bBlockComment = false;
        bReadFromInclude = false;
        bAutoStart = false;
        bScriptOpen = false;
        sIncludeFileName = "";
        bValidScript = false;
        bInstallProcedures = false;
        bENABLE_FULL_LOGGING = false;
        bDISABLE_SCREEN_OUTPUT = false;
        bIsInstallingProcedure = false;
        sHelpID = "";
        sInstallID = "";
        nLine = 0;
        _localDef.reset();
    }

    return;
}


/////////////////////////////////////////////////
/// \brief This member function restarts the
/// currently opened script.
///
/// \return void
///
/////////////////////////////////////////////////
void Script::restart()
{
    _localDef.reset();

    if (bScriptOpen)
    {
        // If the script is already open,
        // clear all flags, close the include
        // and jump to the first glyph
        fScript.clear();
        if (fInclude.is_open())
            fInclude.close();
        fScript.seekg(0);
    }
    else if (sScriptFileName.length())
    {
        // If the script was not open
        // but a file name exists, then
        // open this file
        openScript();
        fScript.clear();
        if (fInclude.is_open())
            fInclude.close();
        fScript.seekg(0);
    }
    return;
}


/////////////////////////////////////////////////
/// \brief This member function closes the script,
///  if the code reached a "return" statement.
///
/// \return void
///
/// This function has to be called from the
/// outside, because this class won't track this
/// command internally.
/////////////////////////////////////////////////
void Script::returnCommand()
{
    if (bScriptOpen)
    {
        if (fLogFile.is_open())
        {
            fLogFile << "--- INSTALLATION TERMINATED SUCCESSFULLY ---" << endl;
            fLogFile << endl << endl;
            fLogFile.close();

            if (!bDISABLE_SCREEN_OUTPUT)
                NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALL_SUCCESS")));
        }

        close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function starts the
/// current installation section.
///
/// \param sScriptCommand string&
/// \param bFirstPassedInstallCommand bool&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::startInstallation(string& sScriptCommand, bool& bFirstPassedInstallCommand)
{
    // Open the installation logfile
    fLogFile.open((sTokens[0][1] + "\\install.log").c_str(), ios_base::in | ios_base::out | ios_base::app);

    if (fLogFile.fail())
        throw SyntaxError(SyntaxError::CANNOT_OPEN_LOGFILE, sScriptCommand, SyntaxError::invalid_position, sTokens[0][1] + "\\install.log");

    // Write the first line
    fLogFile << "--- INSTALLATION " << getTimeStamp(false) << " ---" << endl;

    bFirstPassedInstallCommand = true;

    // Remove the install tag and strip the white spaces
    sScriptCommand = sScriptCommand.substr(9);
    StripSpaces(sScriptCommand);

    if (!sScriptCommand.length())
        return false;

    // Try to find the install information tag set
    if (sScriptCommand.find("<info>") == string::npos)
    {
        sScriptCommand = "";
        return false;
    }
    else
    {
        // If the install information tags were found
        // handle them here
        return handleInstallInformation(sScriptCommand, bFirstPassedInstallCommand);
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// install information tags of the current
/// installation section.
///
/// \param sScriptCommand string&
/// \param bFirstPassedInstallCommand bool&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::handleInstallInformation(string& sScriptCommand, bool& bFirstPassedInstallCommand)
{
    unsigned int nNumereVersion = AutoVersion::MAJOR*100+AutoVersion::MINOR*10+AutoVersion::BUILD;
    unsigned int nRequiredVersion = nNumereVersion;
    bFirstPassedInstallCommand = true;

    // If the current install information string is incomplete
    // (i.e. no "<endinfo>" tag), then search for the corresponding
    // tag in the next lines
    if (sScriptCommand.find("<endinfo>") == string::npos)
    {
        string sTemp;

        // Read lines from the script until the "<endinfo>" tag was found
        while (!fScript.eof())
        {
            getline(fScript, sTemp);
            nLine++;
            StripSpaces(sTemp);

            if (sTemp.find("##") != string::npos)
                sTemp = stripLineComments(sTemp);

            if (sTemp.find("<endinfo>") == string::npos)
                sScriptCommand += " " + sTemp;
            else
            {
                sScriptCommand += " " + sTemp.substr(0, sTemp.find("<endinfo>") + 9);
                break;
            }
        }
    }

    // Ensure that an "<endinfo>" tag was found
    if (sScriptCommand.find("<endinfo>") == string::npos)
    {
        throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, "", "");
    }

    // Extract the install information string and the installation ID
    std::string sInstallInfoString = sScriptCommand.substr(sScriptCommand.find("<info>")+6, sScriptCommand.find("<endinfo>")-sScriptCommand.find("<info>")-6);
    sScriptCommand = sScriptCommand.substr(sScriptCommand.find("<endinfo>")+9);
    sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);

    // Determine, whether the current installation needs additional packages
    if (sInstallInfoString.find("requirepackages=") != string::npos)
    {
        // Get the required packages list
        string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);

        // Read all required packages
        while (sInstallPackages.length())
        {
            // Get the next dependency
            string sPackage = getNextArgument(sInstallPackages, true);

            // Try to find the package in the packages
            // folder first before using the main folder
            if (fileExists(ValidFileName("packages/" + sPackage, ".nscr")))
                sPackage = ValidFileName("packages/" + sPackage, ".nscr");
            else
                sPackage = ValidFileName(sPackage, ".nscr");

            if (!sPackage.length())
                continue;

            // If this is the first package, simply append it
            if (!vInstallPackages.size())
            {
                vInstallPackages.push_back(sScriptFileName);
                vInstallPackages.push_back(sPackage);
            }
            else if (std::find(vInstallPackages.begin(), vInstallPackages.end(), sPackage) == vInstallPackages.end())
                vInstallPackages.push_back(sPackage);
        }
    }

    // Determine, whether a version of NumeRe is required
    if (sInstallInfoString.find("requireversion=") != string::npos)
    {
        string sTemp = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15);

        while (sTemp.find('.') != string::npos)
            sTemp.erase(sTemp.find('.'),1);

        nRequiredVersion = (unsigned int)StrToInt(sTemp);
    }

    // Throw an error, if the current version if NumeRe is too old
    if (nRequiredVersion > nNumereVersion)
        throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sScriptCommand, SyntaxError::invalid_position, toString(nRequiredVersion));

    // Examine the license information
    if (sInstallInfoString.find("license=") != std::string::npos)
    {
        std::string sLicense = getArgAtPos(sInstallInfoString, sInstallInfoString.find("license=")+8);
        NumeReKernel::print(LineBreak(_lang.get("SCRIPT_INSTALL_LICENSE_AGREEMENT", sInstallID, sLicense), NumeReKernel::getInstance()->getSettings()));
        NumeReKernel::printPreFmt("|<- ");

        std::string sAnswer;
        NumeReKernel::getline(sAnswer);

        if (sAnswer.substr(0, 1) != _lang.YES())
        {
            NumeReKernel::print(_lang.get("SCRIPT_INSTALL_ABORT"));
            close();
            return false;
        }
    }

    evaluateInstallInformation(sInstallInfoString, bFirstPassedInstallCommand);

    if (sInstallInfoString.length())
        NumeReKernel::getInstance()->getProcedureInterpreter().declareNewPackage(sInstallInfoString);

    if (!sScriptCommand.length())
        return false;
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function extracts the
/// documentation index entry from the
/// installation section.
///
/// \param sScriptCommand string&
/// \return string
///
/////////////////////////////////////////////////
string Script::extractDocumentationIndex(string& sScriptCommand)
{
    // Extract or search for the documentation index entry
    if (sScriptCommand.find("</helpindex>") != string::npos)
    {
        sScriptCommand.erase(sScriptCommand.find("</helpindex>")+12);
    }
    else
    {
        string sTemp = "";

        // Extract lines until the closing tag of the
        // documentation index entry was found
        while (!fScript.eof())
        {
            getline(fScript, sTemp);
            nLine++;
            StripSpaces(sTemp);

            if (sTemp.find("##") != string::npos)
                sTemp = stripLineComments(sTemp);

            if (sTemp.find("</helpindex>") == string::npos)
                sScriptCommand += sTemp;
            else
            {
                sScriptCommand += sTemp.substr(0,sTemp.find("</helpindex>")+12);
                break;
            }
        }
    }

    // Extract the article ID
    sHelpID = Documentation::getArgAtPos(sScriptCommand, sScriptCommand.find("id=")+3);
    StripSpaces(sHelpID);

    // Ensure that the article ID start with the plugin prefix
    if (sHelpID.substr(0, 5) != "plgn_" && sHelpID.substr(0, 4) != "pkg_")
    {
        sScriptCommand.insert(sScriptCommand.find(sHelpID, sScriptCommand.find("id=")+3), "plgn_");
        sHelpID = "plgn_" + sHelpID;
    }

    // Separate the install ID with doubled angles from the remaining string
    return sInstallID + " <<>>" + sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This member function writes the
/// appended documentation article to the target
/// file.
///
/// \param sScriptCommand string&
/// \return void
///
/////////////////////////////////////////////////
void Script::writeDocumentationArticle(string& sScriptCommand)
{
    tinyxml2::XMLDocument doc;

    // Depending on whether the whole file was written in
    // one line or multiple script lines
    if (sScriptCommand.find("</helpfile>") != std::string::npos)
    {
        sScriptCommand.erase(sScriptCommand.find("</helpfile>")+11);
        sScriptCommand.erase(0, 10);

        doc.Parse(sScriptCommand.c_str(), sScriptCommand.length());
    }
    else
    {
        std::string sBuffer = sScriptCommand.substr(10) + "\n";
        std::string sTemp;

        // Read the contents linewise from the script
        while (!fScript.eof())
        {
            std::getline(fScript, sTemp);
            nLine++;
            StripSpaces(sTemp);

            if (sTemp.find("##") != std::string::npos)
                sTemp = stripLineComments(sTemp);

            // Try to find the end of the current documentation article
            if (sTemp.find("</helpfile>") == std::string::npos)
                sBuffer += sTemp + "\n";
            else
            {
                // Append the last line
                sBuffer += sTemp.substr(0,sTemp.find("</helpfile>"));
                break;
            }
        }

        doc.Parse(sBuffer.c_str(), sBuffer.length());
    }

    tinyxml2::XMLElement* article = doc.FirstChildElement("article");

    while (article)
    {
        if (!sHelpID.length())
        {
            sHelpID = article->Attribute("id");

            // Ensure that the article ID start with the plugin prefix
            if (sHelpID.substr(0, 5) != "plgn_" && sHelpID.substr(0, 4) != "pkg_")
                sHelpID = "plgn_" + sHelpID;
        }

        std::string sId = article->Attribute("id");

        if (sId.substr(0, 5) != "plgn_" && sId.substr(0, 4) != "pkg_")
        {
            sId = "plgn_" + sId;
            article->SetAttribute("id", sId.c_str());
        }

        tinyxml2::XMLElement* keywords = article->FirstChildElement("keywords");

        if (!keywords)
        {
            keywords = doc.NewElement("keywords");
            tinyxml2::XMLElement* keyword = doc.NewElement("keywords");
            keyword->SetText(sInstallID.c_str());
            keywords->InsertFirstChild(keyword);
            article->InsertFirstChild(keywords);
        }

        article = article->NextSiblingElement();
    }

    // create a valid file name
    std::string sHelpfileName = "<>/docs/plugins/" + sHelpID + ".nhlp";
    sHelpfileName = FileSystem::ValidFileName(sHelpfileName, ".nhlp");

    if (doc.SaveFile(sHelpfileName.c_str(), false) != tinyxml2::XML_SUCCESS)
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sHelpfileName);

    NumeReKernel::getInstance()->getProcedureInterpreter().addHelpIndex(sInstallID, sHelpID);
    NumeReKernel::getInstance()->getSettings().addFileToDocumentationIndex(sHelpfileName);
    sScriptCommand.clear();
}


/////////////////////////////////////////////////
/// \brief This member function writes the
/// embedded window layout to the target file.
///
/// \param sScriptCommand std::string&
/// \return void
///
/////////////////////////////////////////////////
void Script::writeLayout(std::string& sScriptCommand)
{
    // create a valid file name
    std::string sLayoutFileName = getArgAtPos(sScriptCommand, sScriptCommand.find_first_not_of(' ', 6));
    sLayoutFileName = FileSystem::ValidFileName(sLayoutFileName, ".nlyt");
    ofstream fLayoutFile(sLayoutFileName.c_str());

    // Remove the file name
    size_t nQuotes = 0;

    for (size_t i = sScriptCommand.find_first_not_of(' ', 6); i < sScriptCommand.length(); i++)
    {
        if (sScriptCommand[i] == '"' && sScriptCommand[i-1] != '\\')
            nQuotes++;

        if (!(nQuotes % 2) && sScriptCommand[i] == ' ')
        {
            sScriptCommand = "layout " + sScriptCommand.substr(i);
            break;
        }
        else if (i+1 == sScriptCommand.length())
            sScriptCommand = "layout";
    }

    // Depending on whether the whole file was written in
    // one line or multiple script lines
    if (sScriptCommand.find("endlayout") != string::npos)
    {
        sScriptCommand.erase(sScriptCommand.find("endlayout")+9);

        // Write the contents to the documentation article file
        if (!fLayoutFile.fail())
            fLayoutFile << sScriptCommand << endl;
        else
        {
            fLayoutFile.close();
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sLayoutFileName);
        }

        fLayoutFile.close();
    }
    else
    {
        // Write the contents linewise to the documentation article file
        if (!fLayoutFile.fail())
        {
            fLayoutFile << sScriptCommand << endl;

            string sTemp;
            size_t nIndent = 1;

            // Read the contents linewise from the script
            while (!fScript.eof())
            {
                getline(fScript, sTemp);
                nLine++;
                StripSpaces(sTemp);

                if (sTemp.substr(0, 8) == "endgroup" || sTemp.substr(0, 9) == "endlayout")
                    nIndent--;

                sTemp.insert(0, nIndent, '\t');

                // Try to find the end of the current documentation article
                if (sTemp.find("endlayout") == string::npos)
                {
                    // Write the current line
                    fLayoutFile << sTemp << endl;
                }
                else
                {
                    // Write the last line
                    fLayoutFile << sTemp.substr(0, sTemp.find("endlayout")+9) << endl;
                    break;
                }

                if (sTemp.substr(nIndent, 5) == "group")
                    nIndent++;
            }
        }
        else
        {
            fLayoutFile.close();
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sLayoutFileName);
        }

        fLayoutFile.close();
    }

    sScriptCommand.clear();
}


/////////////////////////////////////////////////
/// \brief This member function evaluates the
/// flags from the installation information
/// string and also removes unnecessary comments.
///
/// \param sInstallInfoString std::string&
/// \param bFirstPassedInstallCommand bool&
/// \return void
///
/////////////////////////////////////////////////
void Script::evaluateInstallInformation(std::string& sInstallInfoString, bool& bFirstPassedInstallCommand)
{
    if (sInstallInfoString.length())
    {
        // Remove block comments
        if (sInstallInfoString.find("#*") != string::npos && sInstallInfoString.find("*#", sInstallInfoString.find("#*")+2) != string::npos)
        {
            for (unsigned int i = 0; i < sInstallInfoString.length(); i++)
            {
                if (sInstallInfoString.substr(i,2) == "#*")
                {
                    if (sInstallInfoString.find("*#", i+2) == string::npos)
                    {
                        sInstallInfoString.erase(i);
                        break;
                    }

                    for (unsigned int j = i; j < sInstallInfoString.length(); j++)
                    {
                        if (sInstallInfoString.substr(j,2) == "*#")
                        {
                            sInstallInfoString.erase(i,j+2-i);
                            break;
                        }
                    }
                }
            }
        }

        // Evaluate the flag list
        if (findParameter(sInstallInfoString, "flags", '='))
        {
            string sParam = getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, "flags", '=')+5);

            if (sParam.find("ENABLE_FULL_LOGGING") != string::npos)
            {
                bENABLE_FULL_LOGGING = true;
            }

            if (sParam.find("DISABLE_SCREEN_OUTPUT") != string::npos)
                bDISABLE_SCREEN_OUTPUT = true;
        }

        if (!bDISABLE_SCREEN_OUTPUT)
            NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_START_INSTALL")) + " ...");
    }

    // Write the installation information string to the
    // installation logfile
    if (sInstallInfoString.length())
    {
        fLogFile << "Installinfo: " << sInstallInfoString << endl;
            bFirstPassedInstallCommand = false;
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns the next
/// valid line from the currently opened script.
///
/// \param bFirstPassedInstallCommand bool&
/// \return string
///
/////////////////////////////////////////////////
string Script::getNextScriptCommandFromScript(bool& bFirstPassedInstallCommand)
{
    string sScriptCommand;
    string sDocumentationBuffer;

    // Search for the next valid and non-empty line
    // in the current script
    while (!fScript.eof() && !sScriptCommand.length())
    {
        string sCurrentLine;

        // Compose lines, which were broken using the "\\" operator
        do
        {
            getline(fScript, sCurrentLine);
            nLine++;
            StripSpaces(sCurrentLine);

            if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                sScriptCommand.erase(sScriptCommand.length()-2);

            sScriptCommand += sCurrentLine;

            // Add a breakpoint, if the user has set it in the editor
            if (NumeReKernel::getInstance()->getDebugger().getBreakpointManager().isBreakpoint(sScriptFileName, nLine-1) && sScriptCommand.substr(0,2) != "|>")
                sScriptCommand.insert(0, "|> ");
        }
        while (!fScript.eof() && sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\");

        // Ignore empty lines
        if (!sScriptCommand.length())
            continue;

        // Try to find the end of the current block comment
        if (bBlockComment && sScriptCommand.find("*#") != string::npos)
        {
            // Append the block comment to the buffered documentation
            if (sDocumentationBuffer.length())
                sDocumentationBuffer += sScriptCommand.substr(0, sScriptCommand.find("*#")+2) + "\n";

            bBlockComment = false;

            if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
            {
                sScriptCommand.clear();
                continue;
            }
            else
                sScriptCommand.erase(0, sScriptCommand.find("*#")+2);
        }
        else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
        {
            // Append the block comment to the buffered documentation
            if (sDocumentationBuffer.length())
                sDocumentationBuffer += sScriptCommand + "\n";

            sScriptCommand.clear();
            continue;
        }

        // Remove line comments
        if (sScriptCommand.find("##") != string::npos)
        {
            // Buffer documentation comments, which might probably
            // be prefixed
            if (bInstallProcedures && sScriptCommand.substr(0, 3) == "##!")
                sDocumentationBuffer += sScriptCommand + "\n";
            else
                sDocumentationBuffer.clear();

            sScriptCommand = stripLineComments(sScriptCommand);

            if (!sScriptCommand.length())
                continue;
        }

        // If we find the installation section, then either jump over it
        // or execute it, if the user wants to do so
        if (sScriptCommand.substr(0,9) == "<install>" && !bInstallProcedures)
        {
            // jump over the installation section
            while (!fScript.eof())
            {
                getline(fScript, sScriptCommand);
                nLine++;
                StripSpaces(sScriptCommand);
                if (sScriptCommand.substr(0,12) == "<endinstall>")
                    break;
            }

            sScriptCommand.clear();
            continue;
        }
        else if (sScriptCommand.substr(0,9) == "<install>")
        {
            // Execute the installation section
            if (!startInstallation(sScriptCommand, bFirstPassedInstallCommand))
                continue;
        }

        // Get the installation information
        if (sScriptCommand.substr(0,6) == "<info>" && bInstallProcedures)
        {
            if (!handleInstallInformation(sScriptCommand, bFirstPassedInstallCommand))
                continue;
        }

        // Extract the documentation index
        /*if (sScriptCommand.substr(0,11) == "<helpindex>" && bInstallProcedures)
        {
            return extractDocumentationIndex(sScriptCommand);
        }*/

        // Write the documentation articles to their corresponding files
        if (sScriptCommand.substr(0,10) == "<helpfile>" && bInstallProcedures)
        {
            writeDocumentationArticle(sScriptCommand);
            continue;
        }

        // Write window layouts
        if (sScriptCommand.substr(0, 7) == "layout ")
        {
            writeLayout(sScriptCommand);
            continue;
        }

        // End the installation
        if (sScriptCommand.substr(0,12) == "<endinstall>")
        {
            if (fLogFile.is_open())
            {
                fLogFile << "--- INSTALLATION TERMINATED SUCCESSFULLY ---" << endl;
                fLogFile << endl << endl;
                fLogFile.close();
                if (!bDISABLE_SCREEN_OUTPUT)
                    NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALL_SUCCESS")));
            }
            bIsInstallingProcedure = false;
            sScriptCommand = sScriptCommand.substr(12);
            if (!sScriptCommand.length())
                continue;
        }

        // Remove possible block comments
        if (sScriptCommand.find("#*") != string::npos)
        {
            // Buffer documentation comments, which might probably
            // be prefixed
            if (bInstallProcedures && sScriptCommand.substr(0, 3) == "#*!")
                sDocumentationBuffer += sScriptCommand + "\n";
            else
                sDocumentationBuffer.clear();

            sScriptCommand = stripBlockComments(sScriptCommand);

            if (!sScriptCommand.length())
                continue;
        }

        // Try to find the end of the current block comment
        if (bBlockComment && sScriptCommand.find("*#") != string::npos)
        {
            // Append the block comment to the buffered documentation
            if (sDocumentationBuffer.length())
                sDocumentationBuffer += sScriptCommand.substr(0, sScriptCommand.find("*#")+2) + "\n";

            bBlockComment = false;

            if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
            {
                sScriptCommand.clear();
                continue;
            }
            else
                sScriptCommand = sScriptCommand.substr(sScriptCommand.find("*#")+2);
        }
        else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
        {
            // Append the block comment to the buffered documentation
            if (sDocumentationBuffer.length())
                sDocumentationBuffer += sScriptCommand + "\n";

            sScriptCommand.clear();
            continue;
        }
    }

    // close the script, if this is the last command
    if (fScript.eof())
    {
        bLastScriptCommand = true;
        Script::close();
    }

    // If the current command is the start of a procedure definition,
    // append the buffered documentation strings
    if (sScriptCommand.substr(0, 10) == "procedure ")
        return sScriptCommand + sDocumentationBuffer;

    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This member function returns the next
/// valid line from the included script.
///
/// \return string
///
/////////////////////////////////////////////////
string Script::getNextScriptCommandFromInclude()
{
    string sScriptCommand;

    // Search for the next valid and non-empty line
    // in the included script
    while (!fInclude.eof() && !sScriptCommand.length())
    {
        string sCurrentLine;

        // Compose lines, which were broken using the "\\" operator
        do
        {
            getline(fInclude, sCurrentLine);
            nIncludeLine++;
            StripSpaces(sCurrentLine);

            if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                sScriptCommand.erase(sScriptCommand.length()-2);

            sScriptCommand += sCurrentLine;
        }
        while (!fInclude.eof() && sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\");

        // Ignore empty lines
        if (!sScriptCommand.length())
            continue;

        // Ignore non-global installation sections
        if (sScriptCommand.substr(0,9) == "<install>"
            || (findCommand(sScriptCommand).sString == "global" && sScriptCommand.find("<install>") != string::npos))
        {
            while (!fInclude.eof())
            {
                getline(fInclude, sScriptCommand);
                nIncludeLine++;
                StripSpaces(sScriptCommand);

                if (sScriptCommand.substr(0,12) == "<endinstall>"
                    || (findCommand(sScriptCommand).sString == "global" && sScriptCommand.find("<endinstall>") != string::npos))
                    break;
            }

            sScriptCommand.clear();
            continue;
        }

        // Handle block comments
        if (sScriptCommand.find("#*") != string::npos)
        {
            sScriptCommand = stripBlockComments(sScriptCommand);

            if (!sScriptCommand.length())
                continue;
        }

        // Try to find the end of the current block comment
        if (bBlockComment && sScriptCommand.find("*#") != string::npos)
        {
            bBlockComment = false;

            if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
            {
                sScriptCommand = "";
                continue;
            }
            else
                sScriptCommand = sScriptCommand.substr(sScriptCommand.find("*#")+2);
        }
        else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
        {
            sScriptCommand = "";
            continue;
        }

        // Ensure that the relevant commands are available
        if (findCommand(sScriptCommand).sString != "define"
            && findCommand(sScriptCommand).sString != "ifndef"
            && findCommand(sScriptCommand).sString != "ifndefined"
            && findCommand(sScriptCommand).sString != "redefine"
            && findCommand(sScriptCommand).sString != "redef"
            && findCommand(sScriptCommand).sString != "lclfunc"
            && findCommand(sScriptCommand).sString != "global")
        {
            sScriptCommand.clear();
            continue;
        }

        // Depending on the include type, only accept the
        // corresponding commands
        if (nIncludeType == 1
            && findCommand(sScriptCommand).sString != "define"
            && findCommand(sScriptCommand).sString != "ifndef"
            && findCommand(sScriptCommand).sString != "ifndefined"
            && findCommand(sScriptCommand).sString != "redefine"
            && findCommand(sScriptCommand).sString != "redef"
            && findCommand(sScriptCommand).sString != "lclfunc")
        {
            sScriptCommand.clear();
            continue;
        }
        else if (nIncludeType == 2
            && findCommand(sScriptCommand).sString != "global")
        {
            sScriptCommand.clear();
            continue;
        }
    }

    // If this is the last line, close the included file
    if (fInclude.eof())
    {
        fInclude.close();
        nIncludeLine = 0;
        bReadFromInclude = false;
        nIncludeType = 0;
    }

    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// script include syntax ("@SCRIPT") and
/// prepares the included file stream.
///
/// \param sScriptCommand string&
/// \return string
///
/////////////////////////////////////////////////
string Script::handleIncludeSyntax(string& sScriptCommand)
{
    // Only accept the including syntax, if we're currently
    // not from another included file
    if (!bReadFromInclude && sScriptCommand[0] == '@' && sScriptCommand[1] != ' ')
    {
        if (sScriptCommand[1] == '"')
            sIncludeFileName = sScriptCommand.substr(2,sScriptCommand.find('"', 2)-2);
        else
            sIncludeFileName = sScriptCommand.substr(1,sScriptCommand.find(' ')-1);

        // Determine the inclusion type
        if (sScriptCommand.find(':') != string::npos)
        {
            if (sScriptCommand.find("defines", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 1;
            }
            else if (sScriptCommand.find("globals", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 2;
            }
            else if (sScriptCommand.find("procedures", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 3;
            }
        }

        // Extract the actual file name
        if (sIncludeFileName.find(':') != string::npos)
        {
            for (int __i = sIncludeFileName.length()-1; __i >= 0; __i--)
            {
                if (sIncludeFileName[__i] == ':'
                    && (__i > 1
                        || (__i == 1 && sIncludeFileName.length() > (unsigned int)__i+1 && sIncludeFileName[__i+1] != '/')))
                {
                    sIncludeFileName.erase(sIncludeFileName.find(':'));
                    break;
                }
            }
        }

        // Get a valid file name
        if (sIncludeFileName.length())
            sIncludeFileName = FileSystem::ValidFileName(sIncludeFileName, ".nscr");
        else
            return "";

        // Open the include file
        bReadFromInclude = true;
        fInclude.clear();
        fInclude.open(sIncludeFileName.c_str());

        // Ensure that the file is valid
        if (fInclude.fail())
        {
            bReadFromInclude = false;
            fInclude.close();
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sScriptCommand, SyntaxError::invalid_position, sIncludeFileName);
        }
        return "";
    }
    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This private member function handles
/// the definition and replacement of local
/// functions.
///
/// \param sScriptCommand string&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::handleLocalDefinitions(string& sScriptCommand)
{
    // If the current command contains the command "lclfunc",
    // then this is a definition
    if (findCommand(sScriptCommand).sString == "lclfunc")
    {
        _localDef.defineFunc(sScriptCommand.substr(7));
        sScriptCommand.clear();
        return false;
    }
    else
    {
        // Simply replace the current call
        return _localDef.call(sScriptCommand);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function is the main
/// interface to the internal managed script. It
/// will always return the next valid script line.
///
/// \return string
///
/////////////////////////////////////////////////
string Script::getNextScriptCommand()
{
    string sScriptCommand = "";
    bool bFirstPassedInstallCommand = false;

    // Get the next script command
    if (bScriptOpen)
    {
        if (!bReadFromInclude)
        {
            // Get the next script command from the currently opened script
            sScriptCommand = getNextScriptCommandFromScript(bFirstPassedInstallCommand);
        }
        else
        {
            // Get the next include string from the included script
            sScriptCommand = getNextScriptCommandFromInclude();
        }
    }
    else
        return "";

    // Remove line comments from the script command
    if (sScriptCommand.find("##") != string::npos && !bInstallProcedures)
    {
        sScriptCommand = stripLineComments(sScriptCommand);
    }

    // Remove block comments from the script command
    if (sScriptCommand.find("#*") != string::npos && !bInstallProcedures)
        sScriptCommand = stripBlockComments(sScriptCommand);

    // Replace "<this>" path tokens with the current script file path
    while (sScriptCommand.find("<this>") != string::npos)
        sScriptCommand.replace(sScriptCommand.find("<this>"), 6, sScriptFileName.substr(0,sScriptFileName.rfind('/')));

    // Handle the include syntax ("@SOMESCRIPT") and everything, what
    // belongs to it
    sScriptCommand = handleIncludeSyntax(sScriptCommand);

    // Ensure that procedures are not written accidentally
    if (!bInstallProcedures
        && sScriptCommand.find("procedure") != string::npos
        && sScriptCommand.find('$', sScriptCommand.find("procedure")) != string::npos)
        throw SyntaxError(SyntaxError::PROCEDURE_WITHOUT_INSTALL_FOUND, sScriptCommand, SyntaxError::invalid_position);

    // If we're currently installing something, note that
    // down in the log file and store, whether the current
    // code section is a procedure or not
    if (fLogFile.is_open() && bInstallProcedures)
    {
        if (sScriptCommand.find("procedure") != string::npos && sScriptCommand.find('$', sScriptCommand.find("procedure")) != string::npos)
        {
            bIsInstallingProcedure = true;
            fLogFile << ">> Installing: \"" << sScriptCommand.substr(sScriptCommand.find('$'), sScriptCommand.find('(', sScriptCommand.find('$'))-sScriptCommand.find('$')) << "\" ..." << endl;

            if (!bDISABLE_SCREEN_OUTPUT)
            {
                NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALLING_PROC", sScriptCommand.substr(sScriptCommand.find('$'), sScriptCommand.find('(', sScriptCommand.find('$'))-sScriptCommand.find('$')))));
            }
        }

        if (bENABLE_FULL_LOGGING)
        {
            if (bIsInstallingProcedure)
                fLogFile << ">> >> Copying: " << sScriptCommand << " ..." << endl;
            else
                fLogFile << ">> Evaluating: " << sScriptCommand << " ..." << endl;
        }

        if (sScriptCommand.find("endprocedure") != string::npos)
            bIsInstallingProcedure = false;
    }

    // If we're not installing, replace all local functions
    if (!bInstallProcedures)
    {
        if (!handleLocalDefinitions(sScriptCommand))
            return "";
    }

    // Return the script command for evaluation
    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This member function sets the script
/// file name in the internal buffer.
///
/// \param _sScriptFileName string&
/// \return void
///
/////////////////////////////////////////////////
void Script::setScriptFileName(string& _sScriptFileName)
{
    if (_sScriptFileName.length())
        _sScriptFileName = FileSystem::ValidFileName(_sScriptFileName, ".nscr");
    sScriptFileName = _sScriptFileName;
    return;
}


/////////////////////////////////////////////////
/// \brief Returns a shortened version of the
/// current script file name.
///
/// \return string
///
/////////////////////////////////////////////////
string Script::getScriptFileNameShort() const
{
    string sFileName = sScriptFileName;
    unsigned int nPos = -1;
    unsigned int nPos_2 = 0;
    while (sFileName.find('\\') != string::npos)
    {
        sFileName[sFileName.find('\\')] = '/';
    }
    while (sFileName.rfind('/', nPos) != string::npos)
    {
        nPos = sFileName.rfind('/', nPos);
        if (nPos != 0 && nPos-1 != ':')
        {
            nPos_2 = sFileName.rfind('/', nPos-1);
            if (nPos_2 != string::npos)
            {
                sFileName = sFileName.substr(0,nPos_2+1) + ".." + sFileName.substr(nPos);
                nPos = nPos_2;
            }
            else
                break;
        }
        else
            break;
    }
    return sFileName;
}

