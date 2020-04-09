module com.ostrichemulators.prevent {
	requires javafx.controls;
	requires javafx.fxml;
	requires java.base;
	requires java.prefs;
	requires java.json;
	requires org.slf4j;
	requires com.amihaiemil.docker;
	requires com.fasterxml.jackson.annotation;
	requires com.fasterxml.jackson.core;
	requires com.fasterxml.jackson.databind;
	requires com.fasterxml.jackson.datatype.jsr310;
	requires org.apache.commons.lang3;
	requires org.apache.commons.codec;
	requires org.apache.commons.io;

	opens com.ostrichemulators.prevent to javafx.fxml, com.fasterxml.jackson.databind;
	exports com.ostrichemulators.prevent;
}
