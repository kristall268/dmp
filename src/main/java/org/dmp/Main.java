package org.dmp;

import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

public class Main extends Application {
    private TextField OsNameField;
    private TextField OsVersionField;
    private TextField OsArchField;

    @Override
    public void start(Stage stage) {
        OsNameField = createDisplayField();
        OsVersionField = createDisplayField();
        OsArchField = createDisplayField();
        GridPane grid = new GridPane();
        grid.setHgap(10);
        grid.setVgap(10);
        grid.setPadding(new Insets(10));

        // Adding the TextFields
        grid.add(new Label("OS Name:"), 0, 0);
        grid.add(OsNameField, 1, 0);

        grid.add(new Label("OS Version:"), 0, 1);
        grid.add(OsVersionField, 1, 1);

        grid.add(new Label("OS Architecture: "), 0, 2);
        grid.add(OsArchField, 1, 2);

        Button btn_reload = new Button("Get OS Info");
        btn_reload.setPrefWidth(150);
        btn_reload.setStyle("-fx-font-size: 14px; -fx-background-color: #ff9f0a; -fx-text-fill: white;");
        btn_reload.setOnAction(e -> updateHardwareUI());

        VBox root = new VBox(15, grid, btn_reload);
        root.setAlignment(Pos.CENTER);
        root.setPadding(new Insets(15));

        Scene scene = new Scene(root, 350, 220);
        stage.setTitle("Start Point");
        stage.setScene(scene);
        stage.show();
    }

    private void updateHardwareUI() {
        HardwareService.HardwareSnapshot snapshot = HardwareService.getOsSnapshot();
        OsNameField.setText(snapshot.os_name());
        OsVersionField.setText(snapshot.os_version());
        OsArchField.setText(snapshot.os_arch());

    }

    private TextField createDisplayField() {
        TextField field = new TextField("unknown");
        field.setEditable(false);
        field.setAlignment(Pos.CENTER_LEFT);
        return field;
    }

    public static void main(String[] args) {
        launch(args);
    }
}
