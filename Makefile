all:
	g++ -o build/server src/server.cpp -lssl -lcrypto -I/usr/include/openssl
	build/server

debug:
	g++ -o build/server src/server.cpp -lssl -lcrypto -I/usr/include/openssl -g
	gdb --args build/server

build:
	g++ -o build/server src/server.cpp -lssl -lcrypto -I/usr/include/openssl -O3

# https://stackoverflow.com/questions/3016956/how-do-i-install-the-openssl-libraries-on-ubuntu
install-ssl:
	sudo apt install libssl-dev

# https://stackoverflow.com/questions/12871565/how-to-create-pem-files-for-https-web-server
generate-keys:
	mkdir conf
	wget https://raw.githubusercontent.com/anders94/https-authorized-clients/master/keys/ca.cnf
	openssl req -new -x509 -days 9999 -config ca.cnf -keyout ca-key.pem -out ca-cert.pem
	openssl genrsa -out key.pem 4096
	wget https://raw.githubusercontent.com/anders94/https-authorized-clients/master/keys/server.cnf
	openssl req -new -config server.cnf -key key.pem -out csr.pem
	openssl x509 -req -extfile server.cnf -days 999 -passin "pass:password" -in csr.pem -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out cert.pem
	openssl x509 -outform der -in cert.pem -out cert.crt
	rm -rf ./conf
	rm ca-cert.pem
	rm ca-key.pem
	rm ca.cnf
	rm csr.pem
	rm cert.crt
	rm server.cnf

remove-keys:
	rm cert.pem
	rm key.pem