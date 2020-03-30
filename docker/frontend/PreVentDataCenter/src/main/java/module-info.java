module com.ostrichemulators.prevent {
	requires javafx.controls;
	requires javafx.fxml;
	requires java.base;
	requires org.slf4j;

	opens com.ostrichemulators.prevent to javafx.fxml;
	exports com.ostrichemulators.prevent;
}
