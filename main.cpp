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
	void push_front( Cell_t * pCell ){
		assert( is_root() ); 
		assert( pCell->prev == NULL && pCell->next == NULL ); // check for unhook
		pCell->next = next;
		pCell->prev = this;
		if( next ){
			next->prev = pCell;
		}
		next = pCell;
	}
	void unhook(){
		assert( !is_root() );
		prev->next = next;
		if( next )
			next->prev = prev;
		prev = NULL;
		next = NULL;
	}
	int  size(){
		assert( is_root() );
		Cell_t * cur = this;
		int len = 0;
		for( ; cur->next != NULL; cur=cur->next, len++ )
			;
		return len;
	}
};

double ParseFile( char * ifname, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell );
void Update_Gain( int CellID, std::vector<Net_t>&, std::vector<Cell_t>& );
struct Cmptor{
	bool operator()( const int& a, const int& b ) const {
		return a>b;
	}
};

void FM( std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
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
		if( cRegion[0] != cRegion[1] )
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
	std::map<int,Cell_t,Cmptor>::iterator itr;
	for( int i=0; i<vCell.size(); i++ ){
		Bucket[ vCell[i].region ][ vCell[i].gain ].push_front( &vCell[i] );
	}
	/* Check object *
	int count = 0;
	for( itr = Bucket.begin(); itr != Bucket.end(); itr++ )
		count += itr->second.size();
	std::cout<<"Bucket size="<< Bucket.size() <<std::endl;
	std::cout<<"Cell in Bucket= "<< count <<std::endl;
	std::cout<<"vCell.size()= "<< vCell.size() <<std::endl;
	*/
	
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
	**/
	FM( vNet, vCell );
}

void Update_Gain( int CellID, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
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
				if( !vCell[ Net[j] ].Lock )
					vCell[ Net[j] ].gain ++;
			}
		}
		else
		if( Net.cRegion[ To ] == 1 ){
			for( int j=0; j<Net.size(); j++ ){
				if( !vCell[ Net[j] ].Lock && vCell[ Net[j] ].region == To ){
					vCell[ Net[j] ].gain --;
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
				if( !vCell[ Net[j] ].Lock )
					vCell[ Net[j] ].gain --;
			}
		}
		else
		if( Net.cRegion[ From ] == 1 ){
			for( int j=0; j<Net.size(); j++ ){
				if( !vCell[ Net[j] ].Lock && vCell[ Net[j] ].region == From ){
					vCell[ Net[j] ].gain ++;
					break;
				}
			}
		}
	}
	Cell.region = (Cell.region+1)%2;
}

double ParseFile( char * ifname, std::map<std::string,int>& NetMap, std::map<std::string,int>& CellMap, std::vector<Net_t>& vNet, std::vector<Cell_t>& vCell ){
	std::ifstream fin( ifname, std::ios::in );
	double ratio;
	fin>>ratio;
	std::string line, word;
	int nAllNet, nAllCell, nAllConn;
	std::map<std::string,int>::iterator mapitr;
	nAllNet = 0;
	nAllCell= 0;
	nAllConn= 0;
	while( std::getline( fin, line ) ){
		if( line == "" )
			continue;
		std::istringstream isstr( line );
		if( !(isstr>>word) ){
			std::cout<<"missing keyword: NET: " << word <<std::endl;
			exit(1);
		}

		if( !(isstr>>word) ){
			std::cout<<"missing net name"<<std::endl;
			exit(1);
		}
		if( NetMap.find(word) != NetMap.end() ){
			std::cout<<"Duplicate net name: "<< word <<std::endl;
			exit(1);
		}
		NetMap[word]= nAllNet ++ ;

		int nCell = 0;
		for( ; isstr>>word; nCell++, nAllConn++ ){
			if( word == ";" )
				continue;
			if( CellMap.find(word) != CellMap.end() )
				continue;
			CellMap[word] = nAllCell++ ;
		}
		
		if( nCell==1 ){
			std::cout<<"Warning: signal terminal net"<< std::endl;
		}
	}
	fin.clear();
	fin.seekg( 0, std::ios::beg );
	
	fin >> ratio;
	vNet.resize( nAllNet );
	vCell.resize( nAllCell );
	int cNet, cCell;
	cNet = 0;
	cCell= 0;
	for( ; std::getline( fin, line ); cNet++ ){
		if( line == "" )
			continue;
		std::istringstream isstr( line );
		if( !(isstr>>word) ){
			std::cout<<"create: missing keyword: NET"<<std::endl;
			exit(1);
		}

		if( !(isstr>>word) ){
			std::cout<<"create: missing net name: "<< line<<std::endl;
			exit(1);
		}
		mapitr = NetMap.find( word );
		int NetID = mapitr->second;
		vNet[ NetID ].Name = (char*) mapitr->first.c_str();
		for( ; isstr>>word; cCell++ ){
			if( word == ";" )
				continue;
			mapitr = CellMap.find( word );
			int CellID = mapitr->second;
			vCell[ CellID ].Name = (char*) mapitr->first.c_str();
			vCell[ CellID ].push_back( NetID  );
			vNet [ NetID  ].push_back( CellID );
		}
	}
	fin.close();
	return ratio;
}
