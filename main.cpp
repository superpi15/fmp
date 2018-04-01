#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

class Net_t: public std::vector<int> {
public:
	char * Name;
};

class Cell_t{
public:
	char * Name;
};

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
			vNet [ NetID  ].push_back( CellID );
		}	
	}
	fin.close();
	return ratio;
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
	printf("%8.6f\n",ratio);
	for( int i=0; i<vNet.size(); i++ ){
		std::cout<< "NET " << vNet[i].Name<< " ";
		for( int j=0; j<vNet[i].size(); j++ ){
			std::cout << vCell[ vNet[i][j] ].Name <<" ";
		}
		std::cout << ";\n";
	}
}
