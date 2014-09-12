#include <stdio.h>
#include <cstdlib>
#include "pin.H"

#define BILLION 100000

static UINT64 total=0;
static FILE *trace = NULL;

KNOB<UINT64>   KnobFastfwd(KNOB_MODE_WRITEONCE,  "pintool",
			   "fastfwd", "0", "Fast forward cound of the executable that you want to instrument");

ADDRINT InsCount()	
{
  total++;
  return (total >= KnobFastfwd.Value() + BILLION);
}

// Analysis routine to check fast-forward condition
ADDRINT FastForward() 
{
  return (total >= KnobFastfwd.Value()  && total <  KnobFastfwd.Value() + BILLION);//(total >= KnobFastfwd.Value()  && total);
}


VOID RecordMemRead(VOID * ip,ADDRINT addr)
{
/*
  if (addr & 0x80000000)
  {
	  fprintf(stderr, "Addr: 0x%x : MSB is not zero\n", addr);
  }
*/
//  fprintf(trace, "%x\n", addr);
  fwrite(&addr,4,1,trace);
}

VOID RecordMemWritten(VOID *ip, ADDRINT addr)
{
/*
  if (addr & 0x80000000)
  {
	  fprintf(stderr, "Addr: 0x%x : MSB is not zero\n", addr);
  }
*/
  //fprintf(trace, "%x\n", addr);
  fwrite(&addr,4,1,trace);
}

void MyExitRoutine() {
  fprintf(stdout, "Finished\n");
  fflush(stdout);
  fclose(trace);
  exit(0);
}

VOID Fini(INT32 code, VOID *v)
{
  fclose(trace);
}

VOID Instruction(INS ins, VOID *v)
{

  INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)InsCount,IARG_END);  
  INS_InsertThenCall(ins, IPOINT_BEFORE,(AFUNPTR) MyExitRoutine,IARG_END);
  
  UINT32 memOperands = INS_MemoryOperandCount(ins);
  for(UINT32 memOp=0; memOp < memOperands;memOp++)
    {
      if(INS_MemoryOperandIsRead (ins,memOp))
	{
	  INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward,IARG_END);
	  INS_InsertThenPredicatedCall(
				   ins,IPOINT_BEFORE,(AFUNPTR)RecordMemRead,
				   IARG_INST_PTR,
				   IARG_MEMORYOP_EA, memOp,
				   IARG_END);
	}
      if(INS_MemoryOperandIsWritten(ins, memOp))
	{
	  INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward,IARG_END);
	  INS_InsertThenPredicatedCall(
				   ins,IPOINT_BEFORE,(AFUNPTR)RecordMemWritten,
				   IARG_INST_PTR,
				   IARG_MEMORYOP_EA, memOp,
				   IARG_END);
	}
    }
}

INT32 Usage()
{
  PIN_ERROR("This pintool print a trace of memory addresses \n"+KNOB_BASE::StringKnobSummary()+"\n");
  return -1;
}

int main(int argc, char *argv[])
{
  if(PIN_Init(argc,argv)) return Usage();

  total=0;
  //trace = fopen("memtrace.out","w");
  trace = fopen("memtrace.out","wb");

  INS_AddInstrumentFunction(Instruction,0);
  PIN_AddFiniFunction(Fini, 0);

  PIN_StartProgram();
  return 0;
}
