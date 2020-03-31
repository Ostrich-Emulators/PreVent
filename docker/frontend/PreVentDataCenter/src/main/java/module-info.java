module com.ostrichemulators.prevent {
	requires javafx.controls;
	requires javafx.fxml;
	requires java.base;
	requires org.slf4j;
	requires com.fasterxml.jackson.annotation;
	requires com.fasterxml.jackson.core;
	requires com.fasterxml.jackson.databind;
	requires org.apache.commons.lang3;

	opens com.ostrichemulators.prevent to javafx.fxml;
	exports com.ostrichemulators.prevent;
}
