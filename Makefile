default: main.exe history_reader.exe order_reader.exe server.exe

.PRECIOUS: %.o
%.exe: %.o sqlite.o Semaphore.o access_token.o HTTPSRequestHandler.o QueryRunner.o
	g++ -ggdb3 $^ -lsqlite3 -lzmq -lpthread -lyaml-cpp -lboost_iostreams -ljsoncpp -lscipopt -lgmp -lzip -lreadline -lpugixml -lboost_thread -lboost_system -lcppnetlib-client-connections -lcppnetlib-server-parsers -lcppnetlib-uri -lssl -lcrypto -lboost_regex -lPocoNet -lPocoFoundation -lPocoNetSSL -o $@

%.o: %.cpp
	g++ -std=c++14 -ggdb3 -c $< -O3 -march=native -isystem /usr/include/eigen3 -o $@

sqlite.o: sqlite.cpp
	g++ -std=c++14 -ggdb3 -c $< -O3 -march=native -o $@

.PHONY: market_groups.json
market_groups.json:
	curl -o $@ https://esi.evetech.net/latest/markets/groups/

.PHONY: prices.json
prices.json:
	curl -o $@ https://esi.evetech.net/latest/markets/prices/

.PHONY: orders1.json
orders1.json:
	curl -o $@ https://esi.evetech.net/latest/markets/10000002/orders/?page=1

# TODO: https://github.com/esi/esi-docs/blob/master/docs/CREST_to_ESI.md#markettypes
# "iterate all the groups and cache"
.PHONY: types.json
types.json:
	wget -O $@ https://crest-tq.eveonline.com/market/types/

# Nikolai Ardoulin: 95524869 (https://api.eveonline.com/eve/CharacterID.xml.aspx?names=Nikolai%20Ardoulin)
.PHONY: charactersheet1.json
charactersheet1.json:
	./sso.py 'https://esi.evetech.net/latest/characters/{character_id}/skills/' > $@

# page?
.PHONY: blueprints.json
blueprints.json:
	./sso.py 'https://esi.evetech.net/latest/characters/{character_id}/blueprints/' > $@

# page?
.PHONY: assetlist.json
assetlist.json:
	./sso.py 'https://esi.evetech.net/latest/characters/{character_id}/assets/' > $@

.PHONY: industry_jobs.json
industry_jobs.json:
	./sso.py 'https://esi.evetech.net/latest/characters/{character_id}/industry/jobs' > $@

# see citadelReader.py
#.PHONY: stations.json
#stations.json:
#	curl -o $@ 'https://esi.evetech.net/latest/universe/structures/?datasource=tranquility'


.PHONY: skillqueue.json
skillqueue.json:
	./sso.py 'https://esi.evetech.net/latest/characters/{character_id}/skillqueue/' > $@

.PHONY: swagger.json
swagger.json:
	curl -o $@ 'https://esi.evetech.net/latest/swagger.json'

.PHONY: clean
clean:
	rm -f *.o *.exe
