
#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include  <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/uuid/sha1.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace bfs = boost::filesystem;
namespace po = boost::program_options;
class DuplicateFinderCreator;

struct BlockPart {
	std::uint32_t blockValue;
	int blockNumber;
	BlockPart(std::uint32_t value, int num) :
		blockValue(value), blockNumber(num) {

	}

	bool operator==(const BlockPart& p) const {
		return blockValue == p.blockValue && blockNumber == p.blockNumber;
	}
};
// для того чтобы можно было хешировать блок+позицию - blocks.
class StringHash {
public:
	size_t operator()(const BlockPart& p) const {
		boost::hash<std::string> string_hash;
		return string_hash(
			std::to_string(p.blockValue) + "_"
			+ std::to_string(p.blockNumber));
	}
};

// название файла, размер, количество считанных байтов.
struct FileInfo {
	bfs::path filePath;
	int size;
	int readCount;
	int currentBlockNumber;
	FileInfo() :
		filePath(""), size(0), readCount(0), currentBlockNumber(0) {

	}
	FileInfo(std::string fPath, int sz) :
		filePath(fPath), size(sz), readCount(0), currentBlockNumber(0) {
	}
};

// будет хранить блоки и список соответствующих им идентификаторов файлов (хешированный  путь).
using blocksMap = std::unordered_map<BlockPart, std::vector<size_t>, StringHash>;

class DuplicatesFinder {
	// информация об обработанных файлах. ключ - это хешированный путь.
	std::unordered_map<size_t, FileInfo> savedFiles;
	// чтобы по размеру файла находить возможные кандидаты в дубликаты.
	std::unordered_map<int, blocksMap> blocksBySizes;
	// по хешу последнего блока собирать дубликаты
	std::unordered_map<size_t, std::unordered_set<size_t>> duplicates;
	size_t maxBlockLength;

	boost::hash<std::string> string_hash;	
	std::vector<std::string> skipDirs;
	int scanLevel;
	int minFileSize;
	std::string fileMasks;
	std::string hashType;
	int blockSize;



	// читает блок из файла.  возвращает указатель на BlockPart
	std::unique_ptr<BlockPart> readBlock(FileInfo & fileInfo) {

		if (fileInfo.readCount == fileInfo.size) {
			return nullptr;
		}
		bfs::ifstream file(fileInfo.filePath);
		if (fileInfo.readCount > 0) {
			file.seekg(fileInfo.readCount, std::ios_base::beg);
		}

		if (!file.eof()) {
			int size = fileInfo.size - fileInfo.readCount;
			std::vector<char> bufer(blockSize);

			auto sizeToRead = std::min(size, blockSize);
			file.read(&bufer[0], sizeToRead);
			fileInfo.readCount = fileInfo.readCount + sizeToRead;
			fileInfo.currentBlockNumber++;
			// дополняем буфер если надо.
			if (sizeToRead < blockSize) {
				for (auto z = sizeToRead; z < blockSize; z++) {
					bufer[z] = '\0';
				}
			}
			// находим хеш значение для блока
			unsigned int hashValue;
			if (hashType == "sha1") {
				boost::uuids::detail::sha1 sha1;
				sha1.process_bytes(&bufer[0], sizeToRead);
				unsigned digest[5] = {0};
				sha1.get_digest(digest);
				hashValue = 0;
				for (auto t : digest) {
					hashValue += t;
				}
			}
			else
			{
				boost::crc_32_type crc32Hash;
				crc32Hash.process_bytes(&bufer[0], sizeToRead);
				hashValue = crc32Hash.checksum();
			}


			return std::make_unique<BlockPart>(hashValue,
				fileInfo.currentBlockNumber);
		}
		return nullptr;
	}

	void FillDuplicates(std::vector<size_t> equalFiles, BlockPart currentBlock,
		size_t filePathHash, FileInfo fileInfo) {
		std::unique_ptr<BlockPart> anotherBlock = nullptr;
		for (auto fl : equalFiles) {
			auto it = savedFiles.find(fl);
			if (it == savedFiles.end())
				continue;
			// файл прочитан  полностью,  проверяем прочитан ли файл, для которого найдено совпадение.
			if (fileInfo.size == fileInfo.readCount
				&& it->second.size == it->second.readCount) {
				// файлы совпали, заполняем список дубликатов.
				duplicates[currentBlock.blockValue].emplace(fl);
				duplicates[currentBlock.blockValue].emplace(filePathHash);
			}
		}
	}

	// Предаю хешированный путь, информацию о текущем файле, список возможных блоков соответсвующих размеру файла.
	void FindBLocks(size_t &filePathHash, FileInfo &fileInfo,
		blocksMap &blocks) {
		std::vector<size_t> equalFiles(maxBlockLength);
		std::unique_ptr<BlockPart> currentBlock = nullptr;
		std::unique_ptr<BlockPart> anotherBlock = nullptr;
		std::vector<size_t> tempFiles(maxBlockLength);
		while (true) {
			currentBlock = readBlock(fileInfo);
			if (currentBlock == nullptr)
				break;
			// найти считанный блок среди блоков для файлов такого же размера.
			auto block = blocks.find(*currentBlock);
			if (block != blocks.end()) {
				// блок найден
				//  если блок первый, запоминаем  совпавшие файлы
				if (fileInfo.currentBlockNumber == 1) {
					if (fileInfo.size == fileInfo.readCount) {
						FillDuplicates(block->second, *currentBlock,
							filePathHash, fileInfo);
						break;
					}
					equalFiles = block->second;
				}

				else {
					//  сравниваем есть ли файлы, для которых до этого уже были совпадения,
					// формируем новый список совпавших.
					tempFiles.clear();
					for (auto fl1 : equalFiles) {
						for (auto fl2 : block->second) {
							if (fl1 == fl2) {
								tempFiles.push_back(fl2);
							}
						}
					}
					equalFiles = tempFiles;
				}
				// блок найден, добавляем текущий файл в список для данного блока.
				block->second.push_back(filePathHash);
				if (block->second.size() > maxBlockLength) {
					maxBlockLength = block->second.size();
				}

			}
			else {
				// блок не найден и он первый, сохранить блок и выйти
				if (fileInfo.currentBlockNumber == 1) {
					//сохранить прочитанный блок
					blocks[*currentBlock].push_back(filePathHash);
					break;
				}
				// совпавших нет, тогда прочитать блоки из файлов для которых предыдущие блоки файлов совпали
				if (equalFiles.size() != 0) {
					tempFiles.clear();
					blocks[*currentBlock].push_back(filePathHash);
					for (auto fl : equalFiles) {
						auto it = savedFiles.find(fl);
						if (it == savedFiles.end())
							continue;
						anotherBlock = readBlock(it->second);
						if (anotherBlock != nullptr) {
							//сохранить прочитанный блок
							blocks[*anotherBlock].push_back(it->first);
							if (*anotherBlock == *currentBlock) {
								tempFiles.push_back(fl);
							}
						}
					}
					equalFiles = tempFiles;
				}
			}
			//блоки прочитаны, файл прочитан, заполняется список дубликатов.
			if (equalFiles.size() != 0) {
				if (fileInfo.size == fileInfo.readCount) {
					FillDuplicates(equalFiles, *currentBlock, filePathHash,
						fileInfo);
					break;
				}
			}
			if (equalFiles.size() == 0) {
				break;
			}
		}
	}

	// сравнить файлы.
	void compareFiles(std::string dirName) {
		bfs::path dirPath(dirName);
		// сравнить файлы.
	}

	// просмотреть файлы конкретной папки
	void ProcessDirectory(std::string dirName) {
		bfs::path dirPath(dirName);
		try {
			maxBlockLength = 0;
			if (bfs::is_directory(dirPath)) {

				int size;
				bfs::path currentPath;
				std::unique_ptr<BlockPart> firtstBlock = nullptr;
				size_t pathHash;
				blocksMap tempMap;
				boost::regex filter;
				if (fileMasks.size() > 0) {
					filter = boost::regex(fileMasks);
				}


				auto dirIter = bfs::recursive_directory_iterator(dirPath);
				bfs::recursive_directory_iterator endIter;
				std::string currentPathstr;
				auto matchMask = true;
				boost::cmatch matches;
				// просмотр всех файлов директории
				while (dirIter != endIter)
				{
					// учитывается глубина просмотра файлов.

					currentPath = dirIter->path();
					if (dirIter.depth() > scanLevel || std::find(skipDirs.begin(), skipDirs.end(), currentPath) != skipDirs.end()) {
						dirIter.no_push();
						dirIter++;
						continue;
					}

					currentPathstr = currentPath.string();
					// соответствие маске
					matchMask = true;
					if (!fileMasks.empty()) {
						if (boost::regex_search(currentPathstr.c_str(), matches, filter))
						{
							matchMask = true;
						}
						else {
							matchMask = false;
						}
					}

					// если первый файл, не смотрим дальше
					if (!is_regular_file(currentPath) || matchMask == false) {
						dirIter++;
						continue;
					}
					size = bfs::file_size(currentPath);
					if (size <= minFileSize)
					{
						dirIter++;
						continue;
					}

					//  сохраняем информацию о файле.
					pathHash = string_hash(currentPathstr);
					auto savedFileIter = savedFiles.emplace(pathHash,
						FileInfo(currentPathstr, size)).first;

					if (savedFiles.size() == 1) {
						dirIter++;
						continue;
					}
					auto blocksBySizesIter = blocksBySizes.find(size);
					// нет файла с таким же размером, не с чем сравнивать, посмотрим предыдущие файлы.
					if (blocksBySizesIter == blocksBySizes.end()) {
						tempMap.clear();
						for (auto &file : savedFiles) {
							//читаем блок.
							if (file.second.size == size
								&& file.first != pathHash) {
								firtstBlock = readBlock(file.second);
								if (firtstBlock != nullptr)
									tempMap[*firtstBlock].push_back(file.first);
							}
						}
						if (tempMap.size() > 0) {
							FindBLocks(pathHash, savedFileIter->second,
								tempMap);
							blocksBySizes[size] = tempMap;
						}

					}
					else {
						// читаю блоки из текущего файла и сравниваю с просмотренными файлами кандидатами в дубликаты.
						FindBLocks(pathHash, savedFileIter->second,
							blocksBySizesIter->second);
					}
					dirIter++;
				}
			}
			else {
				std::cout << dirPath << " is not a directoryName";
			}
		}
		catch (const bfs::filesystem_error& ex) {
			std::cout << ex.what() << '\n';
		}
	}

	friend class DuplicateFinderCreator;
public:
std::vector<std::string> scanDirs;
	std::vector<std::vector<std::string>> GetDuplicates() {

		if (savedFiles.size() == 0) {
			for (auto dir : scanDirs) {
				ProcessDirectory(dir);
			}

		}
		std::vector<std::vector<std::string>> result(duplicates.size());
		if (savedFiles.size() == 0 || duplicates.size() == 0) {
			return result;
		}

		int i = 0;
		for (auto fileHash : duplicates) {
			for (auto file : fileHash.second) {

				auto savedFileIter = savedFiles.find(file);
				if (savedFileIter != savedFiles.end()) {
					result[i].push_back(
						savedFileIter->second.filePath.string());
				}
			}
			i++;
		}
		return result;
	}

	void printDuplicates() {
		auto res = GetDuplicates();
		if (res.size() == 0) {
			std::cout << "No files to print";
		}

		for (auto dupl : res) {
			for (auto file : dupl) {
				std::cout << file << "\n";
			}
			std::cout << std::endl;
		}

	}
};

class DuplicateFinderCreator {
public:
	static DuplicatesFinder GetDuplicatesFinder(int argsCount, char *av[]) {
		po::options_description desc("Duplicate file options");
		desc.add_options()
			("scanDirs", po::value<std::vector<std::string>>(), "Directory to scan")
			("skipDirs", po::value<std::vector<std::string>>(), "Directory to avoid")
			("scanLevel", po::value<int>(), "Level to scan children directories")
			("minFileSize", po::value<int>(), "Lower bound for file size")
			("fileMasks", po::value<std::string>(), "Mask for files")
			("hashType", po::value<std::string>(), "Hash type")
			("blockSize", po::value<int>(), "Block size to read from file");
		DuplicatesFinder finder;
		po::variables_map vm;
		po::store(po::parse_command_line(argsCount, av, desc), vm);
		po::notify(vm);
		if (vm.count("scanDirs")) {
			finder.scanDirs = vm["scanDirs"].as <std::vector<std::string>>();
		}
		else {
			throw("No scan directory set");
		}

		if (vm.count("skipDirs")) {
			finder.skipDirs = vm["skipDirs"].as <std::vector<std::string>>();
		}

		if (vm.count("scanLevel")) {
			finder.scanLevel = vm["scanLevel"].as <int>();
		}
		else {
			finder.scanLevel = 0;
		}

		if (vm.count("minFileSize")) {
			finder.minFileSize = vm["minFileSize"].as <int>();
		}
		else {
			finder.minFileSize = 1;
		}

		if (vm.count("fileMasks")) {
			//boost::split(finder.fileMasks, vm["fileMasks"].as <std::string>(), boost::is_any_of("|"));
			auto str = vm["fileMasks"].as <std::string>();
			boost::replace_all(str, ".", "\\.");
			finder.fileMasks = std::string("(" + str + ")+");
		}

		if (vm.count("hashType")) {
			finder.hashType = vm["hashType"].as <std::string>();
		}

		if (vm.count("blockSize")) {
			finder.blockSize = vm["blockSize"].as <int>();
		}
		else {
			finder.blockSize = 5;
		}


		return finder;
	}

};
