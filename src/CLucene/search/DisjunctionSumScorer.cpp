#include "CLucene/StdHeader.h"
#include "DisjunctionSumScorer.h"

CL_NS_DEF(search)

DisjunctionSumScorer::DisjunctionSumScorer( DisjunctionSumScorer::ScorersType* subScorers ) : Scorer(  NULL ), subScorers(false)
{
	init( subScorers, 1 );
}

DisjunctionSumScorer::DisjunctionSumScorer( DisjunctionSumScorer::ScorersType* subScorers, int32_t minimumNrMatchers ) : Scorer( NULL ), subScorers(false)
{	
	init( subScorers, minimumNrMatchers );
}

void DisjunctionSumScorer::init( DisjunctionSumScorer::ScorersType* subScorers, int32_t minimumNrMatchers )
{
	
	if ( minimumNrMatchers <= 0 ) {
		// throw exception
	}
	
	if ( subScorers->size() <= 1 ) {
		// throw exception
	}

	this->subScorers = subScorers;
	this->minimumNrMatchers = minimumNrMatchers;
	this->nrScorers = subScorers->size();
	this->scorerDocQueue = NULL;
	
}

DisjunctionSumScorer::~DisjunctionSumScorer()
{
	_CLDELETE( scorerDocQueue );
}

void DisjunctionSumScorer::score( HitCollector* hc )
{
	while( next() ) {
		hc->collect( currentDoc, currentScore );
	}
}

bool DisjunctionSumScorer::next()
{
	if ( scorerDocQueue == NULL ) {
		initScorerDocQueue();
	}
	return ( scorerDocQueue->size() >= minimumNrMatchers ) && advanceAfterCurrent();
}

float_t DisjunctionSumScorer::score()
{
	return currentScore;
}

int32_t DisjunctionSumScorer::doc() const
{
	return currentDoc;
}

int32_t DisjunctionSumScorer::nrMatchers()
{
	return _nrMatchers;
}

bool DisjunctionSumScorer::skipTo( int32_t target )
{
	if ( scorerDocQueue == NULL ) {
		initScorerDocQueue();
	}
	if ( queueSize < minimumNrMatchers ) {
		return false;
	}
	if ( target <= currentDoc ) {
		return true;
	}
	do { 		
		if ( scorerDocQueue->topDoc() >= target ) {
			return advanceAfterCurrent();
		} else if ( !scorerDocQueue->topSkipToAndAdjustElsePop( target )) {
			if ( --queueSize < minimumNrMatchers ) {
				return false;
			}
		}		
	} while ( true );
}

TCHAR* DisjunctionSumScorer::toString()
{
	return NULL;
}

void DisjunctionSumScorer::explain( int32_t doc, Explanation* ret )
{
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: DisjunctionSumScorer::explain");								
}

bool DisjunctionSumScorer::score( HitCollector* hc, int32_t max )
{
	while ( currentDoc < max ) {
		hc->collect( currentDoc, currentScore );
		if ( !next() ) {
			return false;
		}
	}
	return true;
}

bool DisjunctionSumScorer::advanceAfterCurrent()
{
	do {
		
		currentDoc = scorerDocQueue->topDoc();
		currentScore = scorerDocQueue->topScore();
		_nrMatchers = 1;
		
		do { 
			
			if ( !scorerDocQueue->topNextAndAdjustElsePop() ) {
				if ( --queueSize == 0 ) {
					break;
				}
			}
			if ( scorerDocQueue->topDoc() != currentDoc ) {
				break;
			}
			currentScore += scorerDocQueue->topScore();
			_nrMatchers++;
			
		} while( true );
		
		if ( _nrMatchers >= minimumNrMatchers ) {
			return true;
		} else if ( queueSize < minimumNrMatchers ) {
			return false;
		}
		
	} while( true );
}

void DisjunctionSumScorer::initScorerDocQueue()
{
	scorerDocQueue = _CLNEW ScorerDocQueue( nrScorers );
	queueSize = 0;
	
	for ( ScorersType::iterator it = subScorers->begin(); it != subScorers->end(); ++it ) {
		Scorer* scorer = (Scorer*)(*it);
		if ( scorer->next() ) {
			if ( scorerDocQueue->insert( scorer )) {
				queueSize++;
			}
		}
	}
	
}

CL_NS_END