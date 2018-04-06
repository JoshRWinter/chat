#ifndef CSV_H
#define CSV_H

class CSVReader{
public:
	CSVReader(const std::string &e)
		: entry(e)
		, index(0)
	{
		const int len = e.length();
		bool escape = false;
		int field_start = 0;

		for(int i = 0; i < len + 1; ++i){
			const char c = i < len ? e.at(i) : 0;

			if(!escape && c == '\\'){
				escape = true;
			}
			else if((!escape && c == ',') || c == 0){
				fields.push_back(strip(entry.substr(field_start, i - field_start)));
				field_start = i + 1;
			}
			else{
				escape = false;
			}
		}
	}

	~CSVReader(){
		if(index < fields.size()){
			log_error("unread fields on entry \"" + entry + "\"");
			std::abort();
		}
	}

	std::optional<std::string> next(){
		if(index < fields.size())
			return fields.at(index++);
		else
			return std::nullopt;
	}

private:
	std::string strip(const std::string &l){
		std::string line = l;

		for(unsigned i = 0; i < line.size(); ++i){
			const char c = line.at(i);

			if(c == '\\')
				line.erase(line.begin() + i);
		}

		return line;
	}

	std::vector<std::string> fields;
	const std::string &entry;
	unsigned index;
};

class CSVWriter{
public:
	CSVWriter() = default;

	void add(const std::string &field){
		if(record.size() != 0)
			record.push_back(',');

		for(const char c : field){
			if(c == '\\')
				record.append("\\\\");
			else if(c == ',')
				record.append("\\,");
			else
				record.push_back(c);
		}
	}

	const std::string &entry()const{
		return record;
	}

private:
	std::string record;
};

#endif // CSV_H
