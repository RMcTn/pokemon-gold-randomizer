#include <fstream>
#include <vector>
class Rom {

public:
	Rom();
	void randomize_starters(int seed = 0);
	bool load();
	bool save();

private:
	std::fstream file;
	std::vector<uint8_t> rom;

};
