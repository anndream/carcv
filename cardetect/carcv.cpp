#include "carcv.hpp"
#include "det.hpp"

using namespace std;
using namespace cv;

namespace fs = boost::filesystem;

int main(int argc, char** argv) {
	CarCV c;

	cout << "arg1: path of list" << endl;
	cout << "arg2: cascadexml path" << endl;


	CascadeClassifier cascade;
	cascade.load(argv[2]);

	fs::path listPath(argv[1]);

	c.run(listPath, CCV_HAAR_SURF, cascade);


	/* maptest
	map<string, double> mapl;

	for (int i = 1; i <= 5; i++) {
		istringstream source(argv[i]);
		string token;
		string key;
	    double value;
		for (int i = 0; getline( source, token, '=' ); i++) {
			if (i == 0) {
				key = token;
			}
		    istringstream ss(token);
		    ss >> value;

		}
		mapl.insert(pair<string, double>(key, value));
	}

	cout << mapl.size() << endl;

	string s = argv[6];
	bool test = (CarCV::atMap(mapl, s) == (*mapl.end()).second);
	cout << "---------------------" << endl;
	cout << "AtIndex: "<< CarCV::atMap(mapl, s) << endl;
	cout << "Works?: " << test << endl;*/

	return 0;
}

/*
 * Main run method
 * method is from enum of same name
 */
void CarCV::run(fs::path &imgListPath, int method, CascadeClassifier &cascade) {
	fs::path posDirPath = "pos"; //load pos dir path
	fs::path negDirPath = "neg"; //load neg dir path

	list<string> strImgList = CarCV::parseList(imgListPath); //parse image list file to list<string>
	list<CarImg> imgList = CarCV::loadCarImgList(strImgList); //load CarImg objects from the list

	CarCV c; //create an instance of CarCV
	list<CarImg> posCarImgList = c.detect_sortPOS_AND_NEG(&imgList, cascade, posDirPath, negDirPath);//detect and sort objects in images of imgList


	fs::path carsDir = "cars";
	if (!fs::exists(carsDir) || !fs::is_directory(carsDir)) { //create cars dir
		fs::create_directory(carsDir);
	}


	list<list<CarImg> > cars = c.sortUnique(posCarImgList, cascade);

	list<CarImg> carlist;
	const int carsListSize = cars.size();
	double speed;

	for (int i = 0; i < carsListSize; i++) {
		carlist = CarCV::atList(cars, i);
		speed = c.calcSpeed(carlist, CCV_SP_FROMALLFILES);
		cout << "Car speed: " << speed << "km/h" << endl;
	}


}

/*
 * //TODO: to be implemented
 * Returns list of positive images list<CarImg>
 *
 */
list<CarImg> CarCV::detect_sortPOS_AND_NEG(list<CarImg> *imgList, CascadeClassifier &cascade, fs::path &posDirPath, fs::path &negDirPath) {
	list<CarImg> sorted = *imgList;

	return sorted; //temp
}

/*
 * Sort images from posImgList into unique car subdirectiories of carsDir
 * Uses <sarcasm> Ondrej Skopek Sort Algorithm (OSSA) </sarcasm>
 */
list<list<CarImg> > CarCV::sortUnique(list<CarImg> &posCarImgList, CascadeClassifier &cascade) { //TODO: test implementation of super algorithm 3000
	/*const*/ double scale = 1;


	map<CarImg, double> probability; //flushed at every iteration over posImgList

	list<list<CarImg> > cars; //sorted list

	list<double> carProbabilty; //result probabilities for every potential unique car

	for (int i = 0; i < CarCV::listSize(posCarImgList); i++) { //iterate over posImgList
		probability.clear();
		carProbabilty.clear();
		const CarImg sortingCar = CarCV::atList(posCarImgList, i);

		if (i == 0 && cars.size() == 0) { //first iteration
			CarCV::atList(cars, 0).push_back(CarCV::atList(posCarImgList, i));
			continue;
		}

		for (int j = 0; j < CarCV::listSize(cars); j++) { //iterate over the main list of cars
			int k;

			const int carsjSize = CarCV::atList(cars, j).size();
			for (k = 0; k < carsjSize; k++) {
				list<CarImg> curList = CarCV::atList(cars, j);
				const CarImg curCar = CarCV::atList(curList, k);

				Mat sortingCarMat = sortingCar.getImg();
				Mat curCarMat = curCar.getImg();

				const double prob = Det::probability(sortingCarMat, curCarMat, cascade, scale); //input from detection algorithm here

				probability.insert(std::pair<CarImg, double>(curCar, prob));
			}
		}

		int carsSize = cars.size();
		for (int l = 0; l < carsSize; l++) {
			double prob;
			int m;
			const int carslSize = CarCV::atList(cars, l).size();
			for (m = 0; m < carslSize; m++) {
				list<CarImg> li = CarCV::atList(cars, l);
				CarImg t = CarCV::atList(li, m);
				prob += CarCV::atMap(probability, t);
			}
			prob /= CarCV::atList(cars, l).size();
			carProbabilty.push_back(prob);
		}

		int carProbId = CarCV::findMaxIndex(carProbabilty);
		CarCV::atList(cars, carProbId).push_back(CarCV::atList(posCarImgList, i));
	}

	return cars;
}

/*
 * Should return the index of the biggest double in mlist
 * If two are equal, returns the index of the first one
 *
 */
int CarCV::findMaxIndex(list<double> &mlist) { //tested, works
	list<double>::iterator mlistI = mlist.begin();
	double probmax;
	int index;

	for (int i = 0; mlistI != mlist.end();i++) {
		if(*mlistI > probmax) {
			probmax = *mlistI;
			index = i;
		}

		mlistI++;
	}
	return index;
}


/*
 * Calculates speed of given unique car from the list of CarImg
 * Give him a row from the main list<list<CarImg> >
 * Returns a positive double
 * if speed_method isn't recognized, returns -1
 *
 * speed_method is from enum of same name
 */
double CarCV::calcSpeed(list<CarImg> clist, int speed_method) { //TODO: not yet implemented
	if (speed_method == 1) { //CCV_SP_FROMSORTEDFILES
		return 1;
	} else if (speed_method == 2) { //CCV_SP_FROMALLFILES
		return 2;
	}
	else {
		int n = speed_method;
		cerr << "ERROR: Unimplemented method: " << n << endl;
		return -1;
	}
	return 0;
}

/*
 * Save CarImg objects to carDir (USE FOR UNIQUE CARS)
 */
void CarCV::saveCarImgList(list<CarImg> carList, fs::path carDir) { //todo: create a saver for CarImgs

}

/**
 * Save list<CarImg> objects to carsDir
 */
void CarCV::saveCars(list<list<CarImg> > cars, fs::path carsDir) { //TODO: create a saver for carsDir

}

/*
 * Load/parse list<CarImg> objects from carsDir
 */
list<list<CarImg> > CarCV::loadCars(fs::path carsDir) { //TODO: create a loader/parser for carsDir
	list<list<CarImg> > carsList;

	return carsList;
}

/*
 * Load/parse CarImg objects from carList
 */
list<CarImg> CarCV::loadCarImgList(list<string> carList) { //TODO: create a loader/parser for CarImg //add a posdir param here
	list<CarImg> carImgList;

	return carImgList;
}

/*
 * Parses the input file plist into a list<string>
 */
list<string> CarCV::parseList(fs::path &plist) { //tested, should work
	list<string> retlist;

	FILE* f = fopen(plist.c_str(), "rt");
	if(f)
	{
		char buf [1000+1];
		for(int i = 0; fgets( buf, 1000, f ); i++)
		{
			int len = (int)strlen(buf);
			while( len > 0 && isspace(buf[len-1]) ) {
				len--;
			}
			buf[len] = '\0';
			retlist.push_back(buf);
		}
	}
	return retlist;
}

/*
 * List item at index
 * If index is out of bounds, returns *tlist.end()
 */
template <class T>
T CarCV::atList(list<T> &tlist, int index) { //

	typename list<T>::iterator tlistI = tlist.begin();

	for (int i = 0; tlistI != tlist.end();i++) {
			if (i == index) {
				return *tlistI;
			}
			tlistI++;
		}
	return *tlist.end();//*--tlistI; was used for returning the last element anyway
}

/*
 * Length of plist
 * Useless: use plist.size()
 */
template <class P>
int CarCV::listSize(list<P> &plist) { //useless, use plist.size()
	typename list<P>::iterator plistI = plist.begin();
	int i;

	for (i = 0; plistI != plist.end();i++) {
			plistI++;
		}
	return i;
}


/*
 * Map item at index
 * If index is not found in map, returns (*tmap.end()).second
 */
template <class K, class V>
V CarCV::atMap(map<K, V> &tmap, K index) { //tested, works

	typename map<K, V>::iterator tmapI = tmap.begin();
	typename map<K, V>::iterator searching = tmap.find(index);


	for (int i = 0; tmapI != tmap.end();i++) {
			if (tmapI == searching) {
				return (*tmapI).second;
			}
			tmapI++;
		}
	return (*tmap.end()).second;
}

/*
 * Size of pmap
 * Useless, use pmap.size()
 */
template <class K, class V>
int CarCV::mapSize(map<K, V> &pmap) { //useless, use pmap.size()
	typename map<K, V>::iterator pmapI = pmap.begin();
	int i;

	for (i = 0; pmapI != pmap.end();i++) {
			pmapI++;
		}
	return i;
}

void grabKVparams(char **argv) { //just for testing reference, erase later
	for (int i = 1; i <= 5; i++) {
		istringstream source(argv[i]);
		string token;
		string key;
		double n;
		for (int i = 0; getline( source, token, '=' ); i++) {
			if (i == 0) {
				key = token;
			}
		    istringstream ss(token);
		    ss >> n;

		}
		cout << key << endl;
		cout << n << endl;
		// do something with n
		cout << "----------" << endl;
	}
}