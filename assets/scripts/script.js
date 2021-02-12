function myCreateFunction() {
    var table = document.getElementById("myTable");
    var row = table.insertRow(1);
    var cell1 = row.insertCell(0);
    var cell2 = row.insertCell(1);
    var cell3 = row.insertCell(2)
    cell1.innerHTML = "Sara";
    cell2.innerHTML = "Rua Zero";
    cell3.innerHTML = "32";
  }

  function myDeleteFunction() {
    document.getElementById("myTable").deleteRow(0);
  }