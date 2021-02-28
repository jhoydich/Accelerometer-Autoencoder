package main

import (
	"bufio"
	"database/sql"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strings"

	_ "github.com/lib/pq"
	"github.com/tarm/serial"
)

var db *sql.DB

// settings to connect to the database
type dbSettings struct {
	Host     string `json:"host"`
	Port     int    `json:"port"`
	User     string `json:"user"`
	Password string `json:"password"`
	DBName   string `json:"dbName"`
}

func main() {
	// name of current print
	printName := flag.String("print", "N/A", "a name of the current printing part")
	flag.Parse()

	// reading and establishing database connection settings from json file
	var settings dbSettings

	jsonFile, err := os.Open("./settings.json")

	if err != nil {
		fmt.Println(err)
	}

	byteValue, _ := ioutil.ReadAll(jsonFile)

	json.Unmarshal(byteValue, &settings)

	// connecting to db with settings
	db = dbConnect(settings)
	defer db.Close()

	err = db.Ping()
	if err != nil {
		panic(err)
	}

	// setting up serial connection
	c := &serial.Config{Name: "Com3", Baud: 115200}
	stream, err := serial.OpenPort(c)

	if err != nil {
		log.Fatal(err)
	}

	scanner := bufio.NewScanner(stream)

	// scanning for new line
	for scanner.Scan() {
		go dbInsert(scanner.Text(), *printName) // inserting
	}

	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
}

func dbConnect(settings dbSettings) *sql.DB {
	psqlInfo := fmt.Sprintf("host=%s port=%d user=%s "+"password=%s dbname=%s sslmode=disable", settings.Host, settings.Port, settings.User, settings.Password, settings.DBName)

	db, err := sql.Open("postgres", psqlInfo)
	if err != nil {
		panic(err)
	}
	return db
}

func dbInsert(line, printName string) {

	// splitting the data from esp.
	l := strings.Split(line, ",")
	if len(l) != 4 {
		return
	}

	// sql statement to insert into database
	stmt := `INSERT INTO readings (x, y, z, time, print) VALUES ($1, $2, $3, $4, $5)`
	_, err := db.Exec(stmt, l[1], l[2], l[3], l[0], printName)
	if err != nil {
		panic(err)
	}

}
