module com.ostrichemulators.prevent {
    requires javafx.controls;
    requires javafx.fxml;

    opens com.ostrichemulators.prevent to javafx.fxml;
    exports com.ostrichemulators.prevent;
}