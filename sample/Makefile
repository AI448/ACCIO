

.PHONY: test

test: empty.csv.result only_newline.csv.result tiny.csv.result

empty.csv.result: parse_csv
	./parse_csv csv_files/empty.csv >$@ && cat $@

only_newline.csv.result: parse_csv
	./parse_csv csv_files/only_newline.csv >$@ && cat $@

tiny.csv.result: parse_csv
	./parse_csv csv_files/tiny.csv >$@ && cat $@

parse_csv: parse_csv.cpp
	g++ parse_csv.cpp ../ACCIO/CORE/BinaryFileReader.cpp ../ACCIO/CORE/Decoder.cpp -std=c++17 -O2 -g -W -Wall -I../ACCIO -o $@

