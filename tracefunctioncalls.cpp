
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::string;
using std::endl;

/* ================================================================== */
// Global variables 
/* ================================================================== */


std::ostream * out = &cerr;
std::ofstream TraceCallInsn;


ADDRINT targetBase = 0;
ADDRINT targetTop = 0;
BOOL g_ModuleLoaded = FALSE;

ADDRINT g_PrevWriteInstAddr = NULL;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
	"o", "calllog.log", "specify file name for MyPinTool output");



/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{

}


/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
*/
VOID RecordInstruction(ADDRINT InstrPtr, ADDRINT CallTarget)
{

	if ((InstrPtr > targetBase) &&
		(InstrPtr < targetTop)
		)
	{
		g_PrevWriteInstAddr = InstrPtr;
		ADDRINT address = 0x10000000 + (InstrPtr - targetBase) & 0xFFFFFFFF;
		
		ADDRINT callAddress = CallTarget;
		if ( (callAddress > targetBase) && (callAddress < targetTop) )
			callAddress = 0x10000000 + (InstrPtr - targetBase) & 0xFFFFFFFF;
		 
		TraceCallInsn << "Original: " << std::hex << InstrPtr << ", Execution address: " << std::hex << address << ", TargetAddr: " << std::hex << callAddress << endl;
	}
	
}

VOID TraceInstruction(ADDRINT InstrPtr)
{
	if ((InstrPtr > targetBase) &&
		(InstrPtr < targetTop)
		)
	{
		g_PrevWriteInstAddr = InstrPtr;
		ADDRINT address = 0x10000000 + (InstrPtr - targetBase) & 0xFFFFFFFF;
		TraceCallInsn << "Original: " << std::hex << InstrPtr << ", Execution address: " << std::hex << address << endl;
	}
}


/*!
*/
VOID InstrumentInst(INS Ins, VOID *)
{
	if (g_ModuleLoaded) 
	{

		if (INS_Valid(Ins))
		{
			ADDRINT currExecAddr = INS_Address(Ins);

			if ((currExecAddr > targetBase) &&
				(currExecAddr < targetTop)
				)
			{
				if (INS_IsCall(Ins))
				{
					
					if (INS_IsDirectCall(Ins))
					{
						ADDRINT callTargetAddress = NULL;
						callTargetAddress = INS_DirectBranchOrCallTargetAddress(Ins);

						INS_InsertPredicatedCall(
							Ins, IPOINT_BEFORE, (AFUNPTR)RecordInstruction,
							IARG_INST_PTR,
							IARG_ADDRINT, callTargetAddress,
							IARG_END);
					}

					else if (INS_IsIndirectBranchOrCall(Ins)) {
						INS_InsertPredicatedCall(
							Ins, IPOINT_BEFORE, (AFUNPTR)RecordInstruction,
							IARG_INST_PTR,
							IARG_BRANCH_TARGET_ADDR,
							IARG_END);
					}
				}
				
				
			}

		}


	}
}

/*!

*/
VOID ImageLoad(IMG Img, VOID *v)
{
	string loadedLibrary = "";

	if (IMG_Valid(Img)) {
		loadedLibrary = IMG_Name(Img);

		string targetLibrary = "LIBRARY TO TARGET";

		TraceCallInsn << loadedLibrary << endl;

		if (loadedLibrary.find(targetLibrary) != std::string::npos )
		{

			targetBase = IMG_LowAddress(Img);
			targetTop = IMG_HighAddress(Img);
			g_ModuleLoaded = TRUE;

			TraceCallInsn << "========= Target Module Loaded: " << std::hex << targetBase << " , " << std::hex << targetTop << endl;

		}
	}
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{

}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
	// Initialize PIN library. Print help message if -h(elp) is specified
	// in the command line or the command line is invalid 
	if (PIN_Init(argc, argv))
	{
		return Usage();
	}

	TraceCallInsn.open(KnobOutputFile.Value().c_str());
	TraceCallInsn.setf(std::ios::showbase);



	IMG_AddInstrumentFunction(ImageLoad, 0);
	INS_AddInstrumentFunction(InstrumentInst, 0);

	// Register function to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);
	

	cerr << "===============================================" << endl;
	cerr << "This application is instrumented by MyPinTool" << endl;
	if (!KnobOutputFile.Value().empty())
	{
		cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
	}
	cerr << "===============================================" << endl;

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
