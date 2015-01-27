#include "replacement_state.h"

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

/*
** This file implements the cache replacement state. Users can enhance the code
** below to develop their cache replacement ideas.
**
*/


////////////////////////////////////////////////////////////////////////////////
// The replacement state constructor:                                         //
// Inputs: number of sets, associativity, and replacement policy to use       //
// Outputs: None                                                              //
//                                                                            //
// DO NOT CHANGE THE CONSTRUCTOR PROTOTYPE                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
CACHE_REPLACEMENT_STATE::CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol )
{

    numsets    = _sets;
    assoc      = _assoc;
    replPolicy = _pol;

    mytimer    = 0;

    InitReplacementState();
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function initializes the replacement policy hardware by creating      //
// storage for the replacement state on a per-line/per-cache basis.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::InitReplacementState()
{
    // Create the state for sets, then create the state for the ways
    repl  = new LINE_REPLACEMENT_STATE* [ numsets ];

    // ensure that we were able to create replacement state
    assert(repl);

    // Create the state for the sets
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        repl[ setIndex ]  = new LINE_REPLACEMENT_STATE[ assoc ];

        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].LRUstackposition = way;
            // initialize RRVP for RRIP policy
            repl[ setIndex][ way ].RRVP = RRIP_MAX;
            // initialize outcome bit for SHiP-PC
            repl[ setIndex][ way ].outcome = false;
        }
    }

    // PSEL Initialization for DRRIP & D-EAF
    PSEL = 0;
    
    // ------------------------Private Variables per Policy
    // DRRIP
    // Set Dueling Initialization
    if (replPolicy == CRC_REPL_DRRIP) 
    {
        setDuelingType = new UINT32 [numsets];
        std::map<UINT32,UINT32> duel;

        for (UINT32 setIndex=0; setIndex<numsets; setIndex++)
            setDuelingType[setIndex] = SDM_FOLLOWER;

        // Create Leader Sets Randomely
        for (UINT32 iteration=0; iteration<NumLeaderSets; iteration++) {
            UINT32 setNo;
            do { setNo = rand() % numsets;
            } while(duel.find(setNo)!=duel.end());
            if (iteration%2) {
                duel[setNo] = SDM_LEADER_SRRIP;
                setDuelingType[setNo] = SDM_LEADER_SRRIP; 
            }
            else {
                duel[setNo] = SDM_LEADER_BRRIP;
                setDuelingType[setNo] = SDM_LEADER_BRRIP;
            }
        }
    }

    // SHiP-PC
    // This assignment is based on that all signatures 
    // will fill the SHCT - or NumSHCTEnties = 2^NumSigBits
    if (replPolicy == CRC_REPL_SHIP) 
    {
        for (UINT32 entry=0; entry<NumSHCTEnties; entry++)
            SHCT[entry] = 0;
    }

    // D-EAF
    // for the bloom filter, we will use a map, and the key is the counter
    // it is easier to do a search
    counter_EAF = 0;
    // Set Dueling Initialization
    // Duel between LRU and EAF
    if (replPolicy == CRC_REPL_EAF) 
    {
        setDuelingType = new UINT32 [numsets];
        std::map<UINT32,UINT32> duel;

        for (UINT32 setIndex=0; setIndex<numsets; setIndex++)
            setDuelingType[setIndex] = SDM_FOLLOWER;

        // Create Leader Sets Randomely
        for (UINT32 iteration=0; iteration<NumLeaderSetsEAF; iteration++) {
            UINT32 setNo;
            do { setNo = rand() % numsets;
            } while(duel.find(setNo)!=duel.end());
            if (iteration%2) {
                duel[setNo] = SDM_LEADER_LRU;
                setDuelingType[setNo] = SDM_LEADER_LRU; 
            }
            else {
                duel[setNo] = SDM_LEADER_EAF;
                setDuelingType[setNo] = SDM_LEADER_EAF;
            }
        }
    }

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache on every cache miss. The input        //
// arguments are the thread id, set index, pointers to ways in current set    //
// and the associativity.  We are also providing the PC, physical address,    //
// and accesstype should you wish to use them at victim selection time.       //
// The return value is the physical way index for the line being replaced.    //
// Return -1 if you wish to bypass LLC.                                       //
//                                                                            //
// vicSet is the current set. You can access the contents of the set by       //
// indexing using the wayID which ranges from 0 to assoc-1 e.g. vicSet[0]     //
// is the first way and vicSet[4] is the 4th physical way of the cache.       //
// Elements of LINE_STATE are defined in crc_cache_defs.h                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc,
                                               Addr_t PC, Addr_t paddr, UINT32 accessType )
{
    // If no invalid lines, then replace based on replacement policy
    if( replPolicy == CRC_REPL_LRU ) 
    {
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        return Get_Random_Victim( setIndex );
    }
    else if ( replPolicy == CRC_REPL_DRRIP ) 
    {
    	// Victim Selection is Same Acorss all Dueling Policies
    	return Get_RRIP_Victim(setIndex);
    }
    else if ( replPolicy == CRC_REPL_SHIP ) 
    {
        // This is the same as SRRIP or above function
        return Get_SHiP_Victim(setIndex);	
    }
    else if ( replPolicy == CRC_REPL_EAF ) 
    {
        // Victim Selection is the same as LRU, but we need to update EAF
        return Get_EAF_Victim( setIndex, vicSet );   
    } 
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR VICTIM SELECTION FUNCTION HERE
    }

    // We should never get here
    assert(0);

    return -1; // Returning -1 bypasses the LLC
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache after every cache hit/miss            //
// The arguments are: the set index, the physical way of the cache,           //
// the pointer to the physical line (should contestants need access           //
// to information of the line filled or hit upon), the thread id              //
// of the request, the PC of the request, the accesstype, and finall          //
// whether the line was a cachehit or not (cacheHit=true implies hit)         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateReplacementState( 
    UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
    UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit )
{
    // What replacement policy?
    if( replPolicy == CRC_REPL_LRU ) 
    {
        UpdateLRU( setIndex, updateWayID );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        // Random replacement requires no replacement state update
    }
    else if ( replPolicy == CRC_REPL_DRRIP ) 
    {
		//Monitoring Set Dueling
		SetDuelingMonitorDRRIP(setIndex, cacheHit);
		//Update RRIP (both SRRIP and BRRIP)
		UpdateRRIP( setIndex, updateWayID, cacheHit );
    }
    else if ( replPolicy == CRC_REPL_SHIP ) 
    {
    	UpdateSHiP( setIndex, updateWayID, PC, cacheHit );
    }
    else if ( replPolicy == CRC_REPL_EAF ) 
    {
        //Monitoring Set Dueling
        SetDuelingMonitorEAF(setIndex, cacheHit);
        //Update both LRU and EAF
        UpdateEAF ( setIndex, updateWayID, currLine, cacheHit );
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {

    }
    
    
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//////// HELPER FUNCTIONS FOR REPLACEMENT UPDATE AND VICTIM SELECTION //////////
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_LRU_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32   lruWay   = 0;

    // Search for victim whose stack position is assoc-1
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( replSet[way].LRUstackposition == (assoc-1) ) 
        {
            lruWay = way;
            break;
        }
    }

    // return lru way
    return lruWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a random victim in the cache set                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_Random_Victim( UINT32 setIndex )
{
    INT32 way = (rand() % assoc);
    
    return way;
}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function updates the set dueling count PSEL based on which set gets   //
// the miss.											                      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::SetDuelingMonitorDRRIP( UINT32 setIndex, bool cacheHit )
{
	// We only update on misses
	if (!cacheHit)
		return;
	// We do not update on follower sets
	if (setDuelingType[setIndex] == SDM_FOLLOWER)
    	return;
    // Now choose between to types of sets and update PSEL
    if (setDuelingType[setIndex] == SDM_LEADER_SRRIP) 
    {
    	if (PSEL==PSEL_MAX)
    		return;
    	PSEL++;
    	return;
    }
    if (setDuelingType[setIndex] == SDM_LEADER_BRRIP)
    {
    	if (PSEL==0)
    		return;
    	PSEL--;
    	return;
    }

    // If we reach here there was an error
    else
    	cout << "\tTHERE WAS AND ERROR IN SET DUELING MONITOR" << endl;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function updates the set dueling count PSEL based on which set gets   //
// the miss.                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::SetDuelingMonitorEAF( UINT32 setIndex, bool cacheHit )
{
    // We only update on misses
    if (!cacheHit)
        return;
    // We do not update on follower sets
    if (setDuelingType[setIndex] == SDM_FOLLOWER)
        return;
    // Now choose between to types of sets and update PSEL
    if (setDuelingType[setIndex] == SDM_LEADER_LRU) 
    {
        if (PSEL==PSEL_MAX_EAF)
            return;
        PSEL++;
        return;
    }
    if (setDuelingType[setIndex] == SDM_LEADER_EAF)
    {
        if (PSEL==0)
            return;
        PSEL--;
        return;
    }

    // If we reach here there was an error
    else
        cout << "\tTHERE WAS AND ERROR IN SET DUELING MONITOR" << endl;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a the victim in RRIP policies. It searches for the 	  //
// RRVP value of RRIP_MAX, if it was find that is the victim. If not it 	  //
// increases all RRVP by one and try again									  //
//																		      //                                                                           
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_RRIP_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32 rripway = 0;

    // Search for the Victim
    for(UINT32 way=0; way<assoc;)
    {	
    	// Find if there is a Line with RRIP_MAX
    	if( replSet[way].RRVP == RRIP_MAX ) 
    	{
    		rripway = way;
    		break;
    	}

    	way++;
    	// If reaches here, Means There is no RRIP_MAX
    	// So Increase all by One and Retry
    	if (way == assoc) 
    	{
    		for(UINT32 way_second=0; way_second<assoc; way_second++)
    			replSet[way_second].RRVP++;
 			way=0;
    	}
    }
    
    return rripway;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This is for SHiP-PC victim selection based on SRRIP.                       //
// This function finds a the victim in RRIP policies. It searches for the     //
// RRVP value of RRIP_MAX, if it was find that is the victim. If not it       //
// increases all RRVP by one and try again.                                   //
// This is for SHiP-PC. If the line was not hitted after we bring it so       //
// outcome bit will be false. If then we decrement the counter of that        //
// signature.                                                                 //
//                                                                            //                                                                           
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_SHiP_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32 rripway = 0;

    // Search for the Victim
    for(UINT32 way=0; way<assoc;)
    {   
        // Find if there is a Line with RRIP_MAX
        if( replSet[way].RRVP == RRIP_MAX_SHiP ) 
        {
            rripway = way;
            break;
        }

        way++;
        // If reaches here, Means There is no RRIP_MAX
        // So Increase all by One and Retry
        if (way == assoc) 
        {
            for(UINT32 way_second=0; way_second<assoc; way_second++)
                replSet[way_second].RRVP++;
            way=0;
        }
    }

    // Now update the SHCT based on Victim outcome
    if (repl[ setIndex][ rripway ].outcome == false)
    {
        if (SHCT.find(repl[setIndex][rripway].signature) != SHCT.end())
        {
            if (SHCT[repl[setIndex][rripway].signature] != 0)
                SHCT[repl[setIndex][rripway].signature]--;
        }
        else
            cout << "\tTHERE WAS AND ERROR IN SHiP VICTIM SELECTION" << endl;
    }
    
    return rripway;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
// Then we will insert the victim tag into the EAF tabel.                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_EAF_Victim( UINT32 setIndex, const LINE_STATE *vicSet )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32   lruWay   = 0;

    // Search for victim whose stack position is assoc-1
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( replSet[way].LRUstackposition == (assoc-1) ) 
        {
            lruWay = way;
            break;
        }
    }

    //Insert the evicted line tag in EAF
    Addr_t tag_vic = vicSet[lruWay].tag;
    EAF[tag_vic] = counter_EAF;
    counter_EAF++;

    if (counter_EAF == BLOOM_MAX_COUNTER)
    {
        EAF.clear();
        counter_EAF = 0;
    }

    return lruWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the LRU update routine for the traditional        //
// LRU replacement policy. The arguments to the function are the physical     //
// way and set index.                                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateLRU( UINT32 setIndex, INT32 updateWayID )
{
    // Determine current LRU stack position
    UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

    // Update the stack position of all lines before the current line
    // Update implies incremeting their stack positions by one
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
        {
            repl[setIndex][way].LRUstackposition++;
        }
    }

    // Set the LRU stack position of new line to be zero
    repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the RRIP update routine with Hit Priority (HP).	  //
// Also BRRIP with BIOMODAL_PROBABILITY constant.   						  //
// On a hit RRPV will be 0. On a miss it will based on Viction selection we   //
// kick-out the line and insert the new line with RRIP_MAX or with Biomodal   //
// probability with RRIP_MAX-1. We update all type of set in this function.	  //
//																			  //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit )
{
	//On Hit all Dueling Policies to the same
	if (cacheHit)
	{
		repl[ setIndex ][ updateWayID ].RRVP = 0;
		return;
	}  

	//If Miss
	//Update Based on Duels
	//Static RRIP
	//1.Leader Set
	if (setDuelingType[setIndex] == SDM_LEADER_SRRIP)
	{
		repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX-1;
		return;	
	}

	//2.Leader Set
	if (setDuelingType[setIndex] == SDM_LEADER_BRRIP)
	{
		if (rand()%1000 < BIOMODAL_PROBABILITY)
			repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX-1;
		else
			repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX;
		return;
	}

	//3.Follower
	else //(SDM_FOLLOWER)
	{
		//PSEL high shows high misses in SRRIP so we choose
		//BRRIP
		if (PSEL > PSEL_MAX/2)
		{
			if (rand()%1000 < BIOMODAL_PROBABILITY)
				repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX-1;
			else
				repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX;
			return;
		}
		//SRRIP
		else
		{
			repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX-1;
			return;	
		}

	}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  This function implement SHiP-PC update routine which is based on SRRIP    //
//  and signature tracking. Hit Priority (HP) is impelemented. So, on every   //
//  hit RRVP is updated to 0. On a miss after eviction we will update to      //
//  outcome bit of the line and based on the signature counter insert it with //
//  different RRVP.                                                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateSHiP( UINT32 setIndex, INT32 updateWayID, Addr_t PC, bool cacheHit ) {
    //On Hit update RRVP to 0 and increment SHCT
    if (cacheHit)
    {
        //RRVP update
        repl[ setIndex ][ updateWayID ].RRVP = 0;

        //SHCT coutner update
        if (SHCT[repl[ setIndex ][ updateWayID ].signature] == SHCTCtrMax)
            return;
        else
            SHCT[repl[ setIndex ][ updateWayID ].signature]++;

        return;
    } 

    // If miss
    repl[ setIndex ][ updateWayID ].outcome = false;
    repl[ setIndex ][ updateWayID ].signature = SHiP_HASH_FUNC (PC);
    if (SHCT[repl[setIndex][updateWayID].signature] == 0)
        repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX;
    else
        repl[ setIndex ][ updateWayID ].RRVP = RRIP_MAX-1;

}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements EAF update procedure. If there was a miss, we     //
// will look at the new tag, and based on EAF table insert the line. However, //
// EAF table lookup will be based on bloom filters. Bloom filters has a false //
// positive probability which we will implement.                              //
// If there was a hit, will be like LRU.                                      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateEAF( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, bool cacheHit )
{
    // Determine current LRU stack position
    UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

    // On Hit all Dueling policies are same
    if (cacheHit)
    {
        // Update the stack position of all lines before the current line
        // Update implies incremeting their stack positions by one
        for(UINT32 way=0; way<assoc; way++) 
        {
            if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
            {
                repl[setIndex][way].LRUstackposition++;
            }
        }

        // Set the LRU stack position of new line to be zero
        repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
        return;
    }

    // Miss
    // We need to decide based on dueling
    // 1.EAF
    if (setDuelingType[setIndex] == SDM_LEADER_EAF)
    {    
        Addr_t tag_new = currLine[updateWayID].tag;
        // check for tag in EAF
        if (EAF.find(tag_new)!=EAF.end())
        {
            // if there is a hit insert as MRU with porbability bloom filter
            if (rand()%1000 > BLOOM_FALSE_POS_PROB) 
            {
                for(UINT32 way=0; way<assoc; way++) 
                    if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                        repl[setIndex][way].LRUstackposition++;

                repl[ setIndex ][ updateWayID ].LRUstackposition = 0;

                return;
            }
        }
        // Both cases:
        // else improt as biomodal policy as MRU
        if (rand()%1000 < BIOMODAL_PROBABILITY_EAF)
        {
            for(UINT32 way=0; way<assoc; way++) 
                if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                    repl[setIndex][way].LRUstackposition++;

            repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
        } 
        // else as LRU - Nothing to do
    }
    // 2.LRU
    if (setDuelingType[setIndex] == SDM_LEADER_LRU)
    {
        for(UINT32 way=0; way<assoc; way++) 
                if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                    repl[setIndex][way].LRUstackposition++;

            repl[ setIndex ][ updateWayID ].LRUstackposition = 0;   
    }
    // 2.Follower
    if (setDuelingType[setIndex] == SDM_LEADER_LRU)
    {
        //PSEL high shows high misses in LRU so we choose
        //EAF
        if (PSEL > PSEL_MAX_EAF/2)
        {
            Addr_t tag_new = currLine[updateWayID].tag;
            // check for tag in EAF
            if (EAF.find(tag_new)!=EAF.end())
            {
                // if there is a hit insert as MRU with porbability bloom filter
                if (rand()%1000 > BLOOM_FALSE_POS_PROB) 
                {
                    for(UINT32 way=0; way<assoc; way++) 
                        if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                            repl[setIndex][way].LRUstackposition++;

                    repl[ setIndex ][ updateWayID ].LRUstackposition = 0;

                    return;
                }
        }
        // Both cases:
        // else improt as biomodal policy as MRU
        if (rand()%1000 < BIOMODAL_PROBABILITY_EAF)
        {
            for(UINT32 way=0; way<assoc; way++) 
                if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                    repl[setIndex][way].LRUstackposition++;

            repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
        } 
        // else as LRU - Nothing to do
        }
        //LRU
        else
        {
            for(UINT32 way=0; way<assoc; way++) 
                if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
                    repl[setIndex][way].LRUstackposition++;

            repl[ setIndex ][ updateWayID ].LRUstackposition = 0; 
        }

    }
}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  This is the hash fucntion for the SHiP-PC                                 //
//  Implemented in the simplest way - LSB bits                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
UINT32 CACHE_REPLACEMENT_STATE::SHiP_HASH_FUNC (Addr_t PC)
{
    return int(PC & int(pow(2,NumSigBits+1)-1));
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// The function prints the statistics for the cache                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
ostream & CACHE_REPLACEMENT_STATE::PrintStats(ostream &out)
{

    out<<"=========================================================="<<endl;
    out<<"=========== Replacement Policy Statistics ================"<<endl;
    out<<"=========================================================="<<endl;

    // CONTESTANTS:  Insert your statistics printing here

    return out;
    
}

