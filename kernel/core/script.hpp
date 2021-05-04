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


// Header-Datei zur Script-Klasse

#ifndef SCRIPT_HPP
#define SCRIPT_HPP
#include <iostream>
#include <string>
#include <fstream>

#include "io/filesystem.hpp"
#include "utils/tools.hpp"
#include "version.h"
#include "maths/define.hpp"

using namespace std;

class Script : public FileSystem
{
    private:
        fstream fScript;
        fstream fInclude;
        fstream fLogFile;
        string sScriptFileName;
        string sIncludeFileName;
        bool bScriptOpen;
        bool bReadFromInclude;
        bool bValidScript;
        bool bAutoStart;
        bool bLastScriptCommand;
        bool bBlockComment;
        unsigned int nLine;
        unsigned int nIncludeLine;
        int nIncludeType;
        bool bInstallProcedures;
        bool bENABLE_FULL_LOGGING;
        bool bDISABLE_SCREEN_OUTPUT;
        bool bIsInstallingProcedure;
        string sInstallInfoString;
        string sHelpID;
        string sInstallID;
        vector<string> vInstallPackages;
        unsigned int nCurrentPackage;
        FunctionDefinitionManager _localDef;

        string stripLineComments(const string& sLine) const;
        string stripBlockComments(const string& sLine);
        bool startInstallation(string& sScriptCommand, bool& bFirstPassedInstallCommand);
        bool handleInstallInformation(string& sScriptCommand, bool& bFirstPassedInstallCommand);
        string extractDocumentationIndex(string& sScriptCommand);
        void writeDocumentationArticle(string& sScriptCommand);
        void writeLayout(std::string& sScriptCommand);
        void evaluateInstallInformation(bool& bFirstPassedInstallCommand);
        string getNextScriptCommandFromScript(bool& bFirstPassedInstallCommand);
        string getNextScriptCommandFromInclude();
        string handleIncludeSyntax(string& sScriptCommand);
        bool handleLocalDefinitions(string& sScriptCommand);


    public:
        Script();
        Script(const string& _sScriptFileName);
        ~Script();


        string getNextScriptCommand();
        void setScriptFileName(string& _sScriptFileName);
        inline string getScriptFileName() const
            {return sScriptFileName;};
        string getScriptFileNameShort() const;
        inline void setPredefinedFuncs(const string& sPredefined)
        {
            _localDef.setPredefinedFuncs(sPredefined);
        }
        inline unsigned int getCurrentLine() const
            {return bReadFromInclude ? nIncludeLine : nLine;}
        inline bool is_including() const
            {return bReadFromInclude;}
        inline void setAutoStart(bool _bAutoStart)
            {
                bAutoStart = _bAutoStart;
                return;
            }
        void openScript();
        void openScript(string& _sScriptFileName);
        void close();
        void restart();
        void returnCommand();
        inline void setInstallProcedures(bool _bInstall = true)
            {
                bInstallProcedures = _bInstall;
                return;
            }
        inline bool isOpen() const
            {return bScriptOpen;};
        inline bool isValid() const
            {return bValidScript;};
        inline bool getAutoStart() const
            {return bAutoStart;}
        inline bool wasLastCommand()
            {
                if (bLastScriptCommand)
                {
                    bLastScriptCommand = false;
                    return true;
                }
                return false;
            }
        inline bool installProcedures() const
            {return bInstallProcedures;}
        inline string getInstallInfoString()
            {
                std::string cp = sInstallInfoString;
                sInstallInfoString.clear();
                return cp;
            }
};

#endif
