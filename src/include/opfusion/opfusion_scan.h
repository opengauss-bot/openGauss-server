/*
 * Copyright (c) 2020 Huawei Technologies Co.,Ltd.
 *
 * openGauss is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ---------------------------------------------------------------------------------------
 * 
 * opfusion_scan.h
 *     scan operator's definition for bypass executor.
 * 
 * IDENTIFICATION
 *        src/include/opfusion/opfusion_scan.h
 *
 * ---------------------------------------------------------------------------------------
 */

#ifndef SRC_INCLUDE_OPFUSION_OPFUSION_SCAN_H_
#define SRC_INCLUDE_OPFUSION_OPFUSION_SCAN_H_
#include "commands/prepare.h"
#include "executor/nodeIndexonlyscan.h"
#include "lib/stringinfo.h"
#include "nodes/parsenodes.h"
#include "pgxc/pgxcnode.h"
#include "storage/buf.h"
#include "utils/plancache.h"
#include "utils/syscache.h"

struct ParamLoc {
    int paramId;
    int scanKeyIndx;
};

class ScanFusion : public BaseObject {
public:
    ScanFusion();

    ScanFusion(ParamListInfo params, PlannedStmt* planstmt);

    static ScanFusion* getScanFusion(Node* node, PlannedStmt* planstmt, ParamListInfo params);

    void refreshParameter(ParamListInfo params);

    virtual void Init(long max_rows) = 0;

    virtual HeapTuple getTuple() = 0;

    virtual void End(bool isCompleted) = 0;

    virtual bool EpqCheck(Datum* values, const bool* isnull) = 0;

    virtual void setAttrNo() = 0;

    virtual TupleTableSlot* getTupleSlot() = 0;

    ParamListInfo m_params;

    PlannedStmt* m_planstmt;

    Relation m_rel;

    TupleTableSlot* m_reslot;

    TupleDesc m_tupDesc;

    ScanDirection* m_direction;
};

class IndexFusion : public ScanFusion {
public:
    IndexFusion(ParamListInfo params, PlannedStmt* planstmt);

    IndexFusion()
    {}

    void refreshParameterIfNecessary();

    void BuildNullTestScanKey(Expr* clause, Expr* leftop, ScanKey this_scan_key);

    void IndexBuildScanKey(List* indexqual);

    virtual void Init(long max_rows) = 0;

    virtual HeapTuple getTuple() = 0;

    virtual void End(bool isCompleted) = 0;

    virtual void setAttrNo();

    virtual TupleTableSlot* getTupleSlot() = 0;

    bool EpqCheck(Datum* values, const bool* isnull);

    Relation m_index; /* index relation */

    Oid m_reloid; /* relation oid of range table */

    AbsIdxScanDesc m_scandesc;

    List* m_epq_indexqual; /* indexqual list */

    bool m_keyInit; /* true if m_scanKeys has been initialized */

    int m_keyNum; /* num of scan key */

    ScanKey m_scanKeys;

    ParamLoc* m_paramLoc; /* location of m_params, include paramId and the location in indexqual */

    int m_paramNum;

    Datum* m_values;

    bool* m_isnull;

    Datum* m_tmpvals; /* for mapping m_values */

    bool* m_tmpisnull; /* for mapping m_isnull */

    List* m_targetList;

    int16* m_attrno; /* target attribute number, length is m_tupDesc->natts */
};

class IndexScanFusion : public IndexFusion {
public:
    IndexScanFusion()
    {}

    ~IndexScanFusion(){};

    IndexScanFusion(IndexScan* node, PlannedStmt* planstmt, ParamListInfo params);

    void Init(long max_rows);

    HeapTuple getTuple();

    void End(bool isCompleted);

    TupleTableSlot* getTupleSlot();

private:
    struct IndexScan* m_node;
};

class IndexOnlyScanFusion : public IndexFusion {
public:
    IndexOnlyScanFusion()
    {}

    ~IndexOnlyScanFusion(){};

    IndexOnlyScanFusion(IndexOnlyScan* node, PlannedStmt* planstmt, ParamListInfo params);

    void Init(long max_rows);

    HeapTuple getTuple();

    void End(bool isCompleted);

    TupleTableSlot* getTupleSlot();

private:
    struct IndexOnlyScan* m_node;

    Buffer m_VMBuffer;
};

#if 0
class RScanFusion : public BaseObject {
public:
    RScanFusion();

    RScanFusion(ParamListInfo params, PlannedStmt* planstmt);

    static RScanFusion* getScanFusion(Node* node, PlannedStmt* planstmt, ParamListInfo params);

    void refreshParameter(ParamListInfo params);

    virtual void Init(long max_rows) = 0;

    virtual HeapTuple getTuple() = 0;

    virtual void End() = 0;

    virtual bool EpqCheck(Datum* values, const bool* isnull) = 0;

    virtual void setAttrNo() = 0;

    virtual TupleTableSlot* getTupleSlot() = 0;

    PlannedStmt* m_planstmt;

    TupleDesc m_tupDesc;
};

class RIndexFusion : public RScanFusion {
public:
    RIndexFusion(ParamListInfo params, PlannedStmt* planstmt);

    RIndexFusion()
    {}

    void refreshParameterIfNecessary();

    void BuildNullTestScanKey(Expr* clause, Expr* leftop, ScanKey this_scan_key);

    void IndexBuildScanKey(List* indexqual);

    virtual void Init(long max_rows) = 0;

    virtual HeapTuple getTuple() = 0;

    virtual void End() = 0;

    virtual void setAttrNo();

    virtual TupleTableSlot* getTupleSlot() = 0;

    bool EpqCheck(Datum* values, const bool* isnull);

    Oid m_reloid; /* relation oid of range table */

    List* m_epq_indexqual; /* indexqual list */

    int m_keyNum; /* num of scan key */

    ParamLoc* m_paramLoc; /* location of m_params, include paramId and the location in indexqual */

    int m_paramNum; 

    List* m_targetList;

    int16* m_attrno; /* target attribute number, length is m_tupDesc->natts */ 
};

class RIndexScanFusion : public RIndexFusion {
public:
    RIndexScanFusion()
    {}

    ~RIndexScanFusion(){};

    RIndexScanFusion(IndexScan* node, PlannedStmt* planstmt, ParamListInfo params);

    void Init(long max_rows);

    HeapTuple getTuple();

    void End();

    TupleTableSlot* getTupleSlot();

private:
    struct IndexScan* m_node;
};

class RIndexOnlyScanFusion : public RIndexFusion {
public:
    RIndexOnlyScanFusion()
    {}

    ~RIndexOnlyScanFusion(){};

    RIndexOnlyScanFusion(IndexOnlyScan* node, PlannedStmt* planstmt, ParamListInfo params);

    void Init(long max_rows);

    HeapTuple getTuple();

    void End();

    TupleTableSlot* getTupleSlot();

private:
    struct IndexOnlyScan* m_node;
};
#endif
#endif /* SRC_INCLUDE_OPFUSION_OPFUSION_SCAN_H_ */
