/*
 * CTaintAnalysis.h
 *
 *  Created on: 2013-11-10
 *      Author: noundou
 */

#ifndef CTAINTANALYSIS_H_
#define CTAINTANALYSIS_H_


#include <llvm/Pass.h>
#include <llvm/Analysis/CallGraph.h>
#include <dsa/DSGraph.h>
#include <dsa/DataStructure.h>
#include <llvm/InstVisitor.h>

#include <vector>
#include <string>
using std::vector;
using std::string;

#include <set>
using std::set;

#include <map>
using std::map;

using std::pair;

using namespace llvm;

namespace {

#define ENTRY_POINT "main"
#define SOURCE_ARG_DELIM ","

//typedef map<Value *, vector<Instruction &> >  ValueTaintingTable;

/**
 * (taintInfo, argNo)
 */
//typedef pair<bool, unsigned> FunctionParam;

/**
 * This class inherits from InstVisitor only because of the context-sensitive
 * analysis. This is needed so that a call to InsVisitor::visit(Function &)
 * can happen in method 'handleContextCall'.
 *
 * This enables the reuse of visit-methods defined in this class. During
 * the intraprocedural analysis, the visit-methods defined in this class
 * are called by subclasses of CForwardFlowAnalysis.
 *
 */
class CTaintAnalysis : public ModulePass, public InstVisitor<CTaintAnalysis> {
public:
	static char ID;

	void getAnalysisUsage(AnalysisUsage & AU) const;
	virtual bool runOnModule(Module & F);

	CTaintAnalysis();
	~CTaintAnalysis();

	inline vector<Function *> *getAllProcsTPOrder() {
		return &_allProcsTPOrder;
	}

	inline vector<Function *> *getAllProcsRTPOrder() {
			return &_allProcsRTPOrder;
	}

	virtual void visit(Instruction &I);
	virtual void visit(BasicBlock &BB){
	  _super->visit(BB.begin(), BB.end());
	}
	void visitLoadInst(LoadInst &I);
	void visitStoreInst(StoreInst &I);
	void visitCallInst(CallInst &I);
	void visitReturnInst(ReturnInst &I);

	void handleContextCall(CallInst &I, Function &callee);
	void handleSinks(CallInst &I, Function &callee);

	void setDiff(set<Value *> &A, set<Value *> &B, set<Value *> &AMinusB);

	virtual bool merge(BasicBlock *curBB, BasicBlock *succBB);
	void mergeCopyPredOutFlowToInFlow(Instruction &predInst, Instruction &curInst);

	void printIn(Instruction &I);
	void printOut(Instruction &I);

	inline DSGraph* getDSGraph(Function *F) {
		return _functionToDSGraph[F];
	}

	inline void setProcArgTaint(Function *F, unsigned argNo, bool isTainted) {
		vector<bool> *argInfo = _summaryTable[F];
		assert ( (argInfo->size() > argNo) && "Invalid function argument number!" );
		(*argInfo)[argNo] = isTainted;
	}

	inline bool isProcArgTaint(Function *F, unsigned argNo) {
		vector<bool> * argInfo = _summaryTable[F];
		assert ( (argInfo->size() > argNo) && "Invalid function argument number!" );
		return (*argInfo)[argNo];
	}

	/**
	 * Returns 'true' if value 'v' is tainted at the program
	 * point before instruction 'I'.
	 */
	bool isValueTainted(Instruction *I, Value *v);

	inline CallGraph &getCallGraph() {
		return *_cg;
	}

	bool calls(Function *caller, Function *callee);

	Function *getMainFunction() { return _pointerMain; }

	void setIntraWasRun(bool flag) {
		_intraWasRun = flag;
	}

	void setInterRunning(bool flag) {
		_interRunning = flag;
	}

	void setCtxInterRunning(bool flag) {
		_ctxInterRunning = flag;
	}

	Module * getModule() {
		return _module;
	}

private:
	const static string _taintId;
	const static string _taintSourceFile;
	const static string _taintSinkFile;
	const static int _SOURCE_ARG_RET;
	const static int _SOURCE_ARG_INVALID_MIN;
	const static int _FUNCTION_NOT_SOURCE;
	static map<string, int> _taintSources;
	static vector<string> _taintSinks;

	/**
	 * Adds the function with name 'source' as a taint source and
	 * specifies that its parameter at position 'taintedPos' gets
	 * tainted.
	 */
	static void addTaintSource(string &source, unsigned taintedPos);

	/**
	 * Reads the configuration file where functions considered
	 * as taint source are registered.
	 */
	static void readTaintSourceConfig();
	static void readTaintSinkConfig();

	/**
	 * Returns an integer p (p > 0) whenever function with
	 * name 'F' is a taint source. The return value 'p' is the
	 * position of its parameter that gets tainted.
	 *
	 * 'p' has the value '-1' in case function 'F' is not a
	 * taint source.
	 */
	int isTaintSource(string &F);

	/** Has the intraprocedural analysis been run */
	bool _intraWasRun;

	/** Has the interprocedural Context-Insenstive analysis been run */
	bool _interRunning;

	/** Is the interprocedural Context-Insenstive analysis currently running */
	bool _ctxInterRunning;

	/** Pointer to the 'main' function */
	Function *_pointerMain;

	InstVisitor<CTaintAnalysis> *_super;

	Instruction *_predInst;

	Module *_module;

	/**
	 * Map from program funtion signatures as string to
	 * Function pointers
	 */
	map<string, Function*> _signatureToFunc;

	vector<Function *> _allProcsTPOrder;

	vector<Function *> _allProcsRTPOrder;

	/**
	 * Summary table where we store function parameters and
	 * return value taint information
	 */
	map<Function *, vector<bool> *> _summaryTable;

	EQTDDataStructures *_aliasInfo;

	map<Function *, DSGraph *> _functionToDSGraph;

	CallGraph *_cg;

	void getAliases(Value *v, DSGraph *dsg, vector<Value *> &aliases);

	//Data structure representing the analysis flowset data type
	//TODO: use StringMap from llvm
	map<Instruction *, set<Value *> > _IN;
	map<Instruction *, set<Value *> > _OUT;
	//ValueTaintingTable _valTaintInfo;

	void insertToOutFlow(Instruction *I, Value *v);
	void insertToOutFlow(Instruction *I, Value *v, DSGraph *dsg);

	static void log(const string &msg);
};
}


#endif /* CTAINTANALYSIS_H_ */
