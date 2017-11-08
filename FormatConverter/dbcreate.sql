-- we don't do anything with this, but it's nice to have for reference
CREATE TABLE patient (
  id INTEGER PRIMARY KEY,
  patientname VARCHAR( 500 )
);

CREATE TABLE unit (
  id INTEGER PRIMARY KEY,
  name VARCHAR( 25 )
);

CREATE TABLE bed (
  id INTEGER PRIMARY KEY,
  unit_id INTEGER,
  name VARCHAR( 25 )
);

CREATE TABLE file (
  id INTEGER PRIMARY KEY,
  filename VARCHAR( 500 ),
  patient_id INTEGER,
  bed_id INTEGER,
  start INTEGER,
  end INTEGER
);

CREATE TABLE offset (
  file_id INTEGER,
  time INTEGER,
  offset INTEGER
);

CREATE TABLE signal (
  id INTEGER PRIMARY KEY,
  name VARCHAR( 25 ),
  hz FLOAT,
  uom VARCHAR( 25 )
);

CREATE TABLE file_signal (
  file_id INTEGER,
  signal_id INTEGER,
  start INTEGER,
  end INTEGER,
  PRIMARY KEY( file_id, signal_id )
);