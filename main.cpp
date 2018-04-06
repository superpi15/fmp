#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <assert.h>

class Net_t: public std::vector<int> {
public:
	Net_t(): Name(NULL){}
	int cRegion[2];
	char * Name;
};

class Cell_t: public std::vector<int> {
public:
	Cell_t(): region(0), Name(NULL), gain(0), Lock(false), prev(NULL), next(NULL) {}
	bool Lock;
	int region, gain;
	char * Name;
	Cell_t * prev, * next;
	bool is_root(){ return prev == NULL; }
	void cpush_front( Cell_t * pCell ){
		assert( is_root() ); 
		assert( pCell->prev == NULL && pCell->next == NULL ); // check for unhook
		pCell->next = next;
		pCell->prev = this;
		if( next ){
			next->prev = pCell;
		}
		next = pCell;
	}
	void cunhook(){
		assert( !is_root() );
		prev->next = next;
		if( next )
			next->prev = prev;
		prev = NULL;
		next = NULL;
	}
	int  csize(){
		assert( is_root() );
		Cell_t * cur = this;
		int len = 0;
		for( ; cur->next != NULL; cur=cur->next, len++ )
			;
		return len;
	}
	bool cempty(){
		assert( is_root() );
		return next == NULL;
	}
};

double ParseFile( char * ifname, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell );
void Update_Gain( int CellID, std::map<int,Cell_t>* , std::vector<Net_t>&, std::vector<Cell_t>& );

struct Cmptor{
	bool operator()( const int& a, const int& b ) const {
		return a>b;
	}
};

std::map<int,Cell_t,Cmptor>::iterator get_nontrivial( std::map<int,Cell_t,Cmptor>& Map ){
	while( !Map.empty() ){
		if( Map.begin()->second.cempty() )
			Map.erase( Map.begin() );
		else
			return Map.begin();
	}
	return Map.end();
}

void FM( double ratio, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
	// Balance placement
	int cRegion[2];
	cRegion[0] = vCell.size()/2;
	cRegion[1] = vCell.size()- cRegion[0];
	for( int i=0; i<vCell.size(); i++ )
		vCell[i].region = 1;
	for( int i=0; i<vCell.size()/2; i++ )
		vCell[i].region = 0;
	
	for( int i=0; i<vCell.size(); i++ ){
		vCell[i].Lock = false;
		vCell[i].gain = 0;
	}
	// Initial cut size
	int nCut = 0;
	for( int i=0; i<vNet.size(); i++ ){
		int cRegion[2];
		vNet[i].cRegion[0] = 0;
		vNet[i].cRegion[1] = 0;
		for( int j=0; j<vNet[i].size(); j++ ){
			vNet[i].cRegion[ vCell[ vNet[i][j] ].region ] ++;
		}
		if( vNet[i].cRegion[0] != vNet[i].cRegion[1] )
			nCut ++;
		for( int j=0; j<vNet[i].size(); j++ ){
			if( vNet[i].cRegion[  vCell[ vNet[i][j] ].region      ]==1 )
				vCell[ vNet[i][j] ].gain ++;
			if( vNet[i].cRegion[ (vCell[ vNet[i][j] ].region+1)%2 ]==0 )
				vCell[ vNet[i][j] ].gain --;
		}
	}
	
	// Pu object onto Bucket
	std::map<int,Cell_t,Cmptor> Bucket[2];
	std::map<int,Cell_t,Cmptor>::iterator itr, itrs[2];
	for( int i=0; i<vCell.size(); i++ ){
		Bucket[ vCell[i].region ][ vCell[i].gain ].cpush_front( &vCell[i] );
	}
	/* Check object *
	int count = 0;
	for( itr = Bucket.begin(); itr != Bucket.end(); itr++ )
		count += itr->second.size();
	std::cout<<"Bucket size="<< Bucket.size() <<std::endl;
	std::cout<<"Cell in Bucket= "<< count <<std::endl;
	std::cout<<"vCell.size()= "<< vCell.size() <<std::endl;
	*/
	std::cout<< nCut<< std::endl;
	int Bound[2];
	int MinCut = nCut;
	Bound[0] = int((double) (0.5-ratio)*vCell.size());
	Bound[1] = int((double) (0.5+ratio)*vCell.size());
	for( int i=0; i<vCell.size(); i++ ){
		itrs[0] = get_nontrivial( Bucket[0] );
		itrs[1] = get_nontrivial( Bucket[1] );
		itr = (itrs[0]!=Bucket[0].end() && itrs[1]!=Bucket[1].end())
			? ( ( itrs[0]->first > itrs[1]->first )? itrs[0]: itrs[1] )
			: ( itrs[0] != Bucket[0].end()? itrs[0]: itrs[1] );
		assert( itr != Bucket[0].end() && itr != Bucket[1].end() );
		Cell_t& Cell = *itr->second.next;
		int cRegionNext[2];
		cRegionNext[0] = cRegion[0] + ( Cell.region==0? -1: +1 );
		cRegionNext[1] = cRegion[1] + ( Cell.region==1? -1: +1 );
		//std::cout<< Bound[0]<<":"<<Bound[1]<<std::endl;
		//std::cout<< cRegionNext[0] <<":"<< cRegionNext[1]<<std::endl;
		if( Bound[0] <= (cRegionNext[0] ) && cRegionNext[1] <= Bound[1] ){
			nCut -= itr->first;
			//std::cout<< nCut<<std::endl;
			if( nCut < MinCut ) MinCut = nCut;
			Update_Gain( &Cell- vCell.data(), (std::map<int,Cell_t>*) Bucket, vNet, vCell );
			cRegion[0] = cRegionNext[0];
			cRegion[1] = cRegionNext[1];
		} else {
			Cell.cunhook();
			Cell.Lock = true;
		}

		//Cell.cunhook();
		//if( itr->second.cempty() )\
			Bucket[ Cell.region ].erase( itr );
	}
	std::cout << nCut<< ":" << MinCut <<std::endl;
}

int main( int argc, char * argv[] ){
	if( argc<2 ){
		std::cout <<"./prog [input] [output]" << std::endl;
		return 0;
	}
	char * ifname, * ofname;
	ifname= ( argc>=2 )? argv[1]: NULL;
	ofname= ( argc>=3 )? argv[2]: NULL;
	std::map<std::string,int> NetMap, CellMap;
	std::vector<Net_t> vNet;
	std::vector<Cell_t> vCell;
	double ratio = ParseFile( ifname, NetMap, CellMap, vNet, vCell );
	/**
	printf("%8.6f\n",ratio);
	for( int i=0; i<vNet.size(); i++ ){
		std::cout<< "NET " << vNet[i].Name<< " ";
		for( int j=0; j<vNet[i].size(); j++ ){
			std::cout << vCell[ vNet[i][j] ].Name <<" ";
		}
		std::cout << ";\n";
	}
	/**/
	FM( ratio, vNet, vCell );
}

void Update_Gain( int CellID, std::map<int,Cell_t>* Bucket, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
	Cell_t& Cell = vCell[CellID];
	//Lock the base cell and complement its block;
	Cell.Lock = true;
	int From, To;
	From =  Cell.region;
	To   = (Cell.region+1)%2;
	for( int i=0; i<Cell.size(); i++ ){
		Net_t& Net = vNet[ Cell[i] ];
		//Check critical nets before the move
		if( Net.cRegion[ To ] == 0 ){
			for( int j=0; j<Net.size(); j++ ){
				Cell_t& CellOp = vCell[ Net[j] ];
				if( ! CellOp.Lock ){
					CellOp.cunhook();
					CellOp.gain ++;
					Bucket[ CellOp.region ][ CellOp.gain ].cpush_front( &CellOp );
				}
			}
		}
		else
		if( Net.cRegion[ To ] == 1 ){
			for( int j=0; j<Net.size(); j++ ){
				Cell_t& CellOp = vCell[ Net[j] ];
				if( !CellOp.Lock && CellOp.region == To ){
					CellOp.cunhook();
					CellOp.gain --;
					Bucket[ CellOp.region ][ CellOp.gain ].cpush_front( &CellOp );
					break;
				}
			}
		}
		
		/* change F(n) and T(n) to reflect the move */
		Net.cRegion[ From ] --;
		Net.cRegion[ To   ] ++;
		/* check for critical nets after the move */
		if( Net.cRegion[ From ] == 0 ){
			for( int j=0; j<Net.size(); j++ ){
				Cell_t& CellOp = vCell[ Net[j] ];
				if( !CellOp.Lock ){
					CellOp.cunhook();
					CellOp.gain --;
					Bucket[ CellOp.region ][ CellOp.gain ].cpush_front( &CellOp );
				}
			}
		}
		else
		if( Net.cRegion[ From ] == 1 ){
			for( int j=0; j<Net.size(); j++ ){
				Cell_t& CellOp = vCell[ Net[j] ];
				if( !CellOp.Lock && CellOp.region == From ){
					CellOp.cunhook();
					CellOp.gain ++;
					Bucket[ CellOp.region ][ CellOp.gain ].cpush_front( &CellOp );
					break;
				}
			}
		}
	}
	Cell.region = (Cell.region+1)%2;
	Cell.cunhook();
}

void ParseNet( std::istream& istr, int& nAllNet, int& nAllCell, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap ){
	std::string word;
	if( !(istr>>word) ){
		std::cout<<"missing net name" <<std::endl;
		exit(1);
	}
	if( NetMap.find(word) != NetMap.end() ){
		std::cout<<"Duplicate net name: "<< word <<std::endl;
		exit(1);
	}
	NetMap[word]= nAllNet ++ ;
	const char * NetName =  NetMap.find( word )->first.c_str();
	int nCell = 0;
	while( istr >> word ){
		if( word == ";" )
			break;
		nCell++;
		if( CellMap.find(word) != CellMap.end() )
			continue;
		CellMap[word] = nAllCell++ ;
	}
	if( nCell==1 ){
		std::cout<<"Warning: signal terminal net: "<< NetName<< std::endl;
	}


}

void BuildNet( std::istream& istr, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
	std::string word;
	std::map<std::string,int>::iterator mapitr;
	if( !(istr>>word) ){
		std::cout<<"create: missing net name: "<<std::endl;
		exit(1);
	}
	mapitr = NetMap.find( word );
	int NetID = mapitr->second;
	assert( NetID < vNet.size() );
	vNet[ NetID ].Name = (char*) mapitr->first.c_str();
	while( istr>>word ){
		if( word == ";" )
			break;
		mapitr = CellMap.find( word );
		int CellID = mapitr->second;
		assert( CellID < vCell.size() );
		vCell[ CellID ].Name = (char*) mapitr->first.c_str();
		vCell[ CellID ].push_back( NetID  );
		vNet [ NetID  ].push_back( CellID );
	}
}


double ParseFile( char * ifname, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
	std::ifstream fin( ifname, std::ios::in );
	int cLine = 0;
	double ratio;
	fin>>ratio;
	std::string line, word;
	int nAllNet, nAllCell, nAllConn;
	std::map<std::string,int>::iterator mapitr;
	nAllNet = 0;
	nAllCell= 0;
	nAllConn= 0;
	bool OpenNet = false;
	while( fin >> word ){			
		if( word != "NET" ){
			std::cout << "Undefined sequence: "<<word << std::endl;
			exit(1);
		}
		ParseNet( fin, nAllNet, nAllCell, NetMap, CellMap );
	}
	fin.clear();
	fin.seekg( 0, std::ios::beg );
	
	fin >> ratio;
	vNet.resize( nAllNet );
	vCell.resize( nAllCell );
	OpenNet = false;
	while( fin >> word ){
		if( word != "NET" ){
			std::cout << "Undefined sequence: "<<word << std::endl;
			exit(1);
		}
		BuildNet( fin, NetMap, CellMap, vNet, vCell );
	}
	fin.close();
	return ratio;
}
