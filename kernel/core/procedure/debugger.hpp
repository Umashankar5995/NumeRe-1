/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2016  Erik Haenel et al.

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

// class: Debugger
// Kann aktiviert werden, um zusätzliche Informationen aus einem "throw" in einer Prozedur zu ziehen

#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <vector>
#include <string>
#include <map>
#include "../ui/error.hpp"
using namespace std;

string toString(int);
inline string toString(unsigned int n)
{
    return toString((int)n);
}
string toString(double,int);
// stacktrace
// line number
// var vals/names
// erratic command
// erratic module
class NumeReDebugger
{
    private:
        vector<string> vStackTrace;
        unsigned int nLineNumber;
        string sErraticCommand;
        string sErraticModule;
        map<string,double> mLocalVars;
        map<string,string> mLocalStrings;
        map<string,string> mLocalTables;
        //map<string,string> mVarMap;
        bool bAlreadyThrown;

    public:
        NumeReDebugger();

        inline bool validDebuggingInformations()
            {return bAlreadyThrown;}
        inline unsigned int getLineNumber()
            {return nLineNumber;}
        inline string getErrorModule()
            {return sErraticModule;}
        inline size_t getStackSize()
            {return vStackTrace.size();}
        void reset();
        void resetBP();
        void pushStackItem(const string& sStackItem);
        void popStackItem();

        void gatherInformations(string** sLocalVars,
                                unsigned int nLocalVarMapSize,
                                double* dLocalVars,
                                string** sLocalStrings,
                                unsigned int nLocalStrMapSize,
                                string** sLocalTables,
                                unsigned int nLocalTableMapSize,
                                const string& _sErraticCommand,
                                const string& _sErraticModule,
                                unsigned int _nLineNumber);

        void gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string,string>& mVarMap, double** vVarArray, string* sVarArray, int nVarArray);

        vector<string> getModuleInformations();
        vector<string> getStackTrace();
        vector<string> getNumVars();
        vector<string> getStringVars();
        vector<string> getTables();
};


#endif // DEBUGGER_HPP

