#ifndef REPL_STATE_H
#define REPL_STATE_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is distributed as part of the Cache Replacement Championship     //
// workshop held in conjunction with ISCA'2010.                               //
//                                                                            //
//                                                                            //
// Everyone is granted permission to copy, modify, and/or re-distribute       //
// this software.                                                             //
//                                                                            //
// Please contact Aamer Jaleel <ajaleel@gmail.com> should you have any        //
// questions                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cassert>
#include <map>
#include <cmath>
#include "utils.h"
#include "crc_cache_defs.h"

//General Defines
#define K   1024
#define M   1000000

//DRRIP Defines
#define NumLeaderSets   64
#define RRIP_MAX        3
#define PSEL_MAX        15
#define BIOMODAL_PROBABILITY    31   //[1 means 0.1%/10 means 1%] of all times

//SHiP Defines
#define RRIP_MAX_SHiP   3
#define NumSHCTEnties   16 * K 		//As paper: it uses direct mapping - Also following paper all comparison is done with unlimited SHCT
#define NumSigBits      14  		//As paper said: 14 bit PC (I used LSB)
#define SHCTCtrMax  	3 			//As paper said: 3-bit saturating counter for default config

//EAF Defines
#define BLOOM_FALSE_POS_PROB		21   	//Based on paper alpha=8 - [1 means 0.1%/10 means 1%] of all times
#define BLOOM_MAX_COUNTER			64 * K 	//Based on paper is the same as number of blocks in the cache
#define BIOMODAL_PROBABILITY_EAF	15		//Based on paper 1/64 - [1 means 0.1%/10 means 1%] of all times
#define NumLeaderSetsEAF   			64
#define PSEL_MAX_EAF        		15

// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_DRRIP      = 2,
    CRC_REPL_SHIP       = 3,
    CRC_REPL_EAF		= 4,	//D-EAF
    CRC_REPL_CONTESTANT = 5
} ReplacemntPolicy;

// Set Type for Dueling DRRIP
typedef enum
{
    SDM_LEADER_SRRIP    = 0,
    SDM_LEADER_BRRIP    = 1,
    SDM_FOLLOWER        = 2,
    SDM_LEADER_LRU	    = 3,
    SDM_LEADER_EAF		= 4
} TypeSetForDuelingDRRIP;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;

    // DRRIP & SHiP-PC
    UINT32 RRVP;

    // SHiP-PC
    bool outcome;
    UINT32 signature;


} LINE_REPLACEMENT_STATE;


// The implementation for the cache replacement policy
class CACHE_REPLACEMENT_STATE
{

  private:
    UINT32 numsets;
    UINT32 assoc;
    UINT32 replPolicy;
    
    LINE_REPLACEMENT_STATE   **repl;

    COUNTER mytimer;  // tracks # of references to the cache

    // DRRIP
    UINT32  *setDuelingType;		// keep the leader sets and follower based on above enum
    UINT32  PSEL;					// counter for set dueling

    // SHiP-PC
    std::map<UINT32, UINT32> SHCT;	// signature history counter table <signature, counter>

    //EAF
    std::map<Addr_t,UINT32> EAF;	// Evicted address buffer - we use this to simulate Bloom filter in EAF
    UINT32 counter_EAF;

  public:

    CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol );

    INT32  GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType );
    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID );

    void   SetReplacementPolicy( UINT32 _pol ) { replPolicy = _pol; } 
    void   IncrementTimer() { mytimer++; } 

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
                                   UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit );

    ostream&   PrintStats( ostream &out);

  private:
    
    void   InitReplacementState();

    INT32  Get_Random_Victim( UINT32 setIndex );
    INT32  Get_LRU_Victim( UINT32 setIndex );
    INT32  Get_RRIP_Victim( UINT32 setIndex );
    INT32  Get_SHiP_Victim( UINT32 setIndex );
    INT32  Get_EAF_Victim( UINT32 setIndex, const LINE_STATE *vicSet );
    UINT32 SHiP_HASH_FUNC (Addr_t PC);

    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );
    void   UpdateRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );
    void   UpdateRRIP( UINT32 setIndex, INT32 updateWayID, Addr_t PC, bool cacheHit );
    void   UpdateSHiP( UINT32 setIndex, INT32 updateWayID, Addr_t PC, bool cacheHit );
    void   UpdateEAF( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, bool cacheHit );

    void   SetDuelingMonitorDRRIP( UINT32 setIndex, bool cacheHit );
    void   SetDuelingMonitorEAF( UINT32 setIndex, bool cacheHit );
};


#endif
