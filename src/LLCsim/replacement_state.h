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
#include "utils.h"
#include "crc_cache_defs.h"

//DRRIP Defines
#define NumLeaderSets   64
#define RRIP_MAX        3
#define PSEL_MAX        15
#define BIOMODAL_PROBABILITY    10   //[1 means 0.1%/10 means 1%] of all times

// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_DRRIP      = 2,
    CRC_REPL_SHIP       = 3,
    CRC_REPL_CONTESTANT = 4
} ReplacemntPolicy;

// Set Type for Dueling DRRIP
typedef enum
{
    SDM_LEADER_SRRIP    = 0,
    SDM_LEADER_BRRIP    = 1,
    SDM_FOLLOWER        = 2
} TypeSetForDuelingDRRIP;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;

    // CONTESTANTS: Add extra state per cache line here
    UINT32 RRVP;

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

    // CONTESTANTS:  Add extra state for cache here
    UINT32  *setDuelingType;
    UINT32  PSEL;

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
    INT32  Get_RRIP_Victim( UINT32 setIndex );
    INT32  Get_LRU_Victim( UINT32 setIndex );

    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );
    void   UpdateRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );
    void   SetDuelingMonitorDRRIP( UINT32 setIndex, bool cacheHit );
};


#endif
