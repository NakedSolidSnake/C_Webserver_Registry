class Person {
    id;
    name;
    address;
    age;
}

var id;

var table = document.getElementById('myTable');

function splitObject(objects) {
    var splitedObjects = objects.split(';')
    return splitedObjects;
}

function createObject(values, object) {
    if (values.length > 0) {
        var fields = values.split(',');
        object.id = fields[0];
        object.name = fields[1];
        object.address = fields[2];
        object.age = fields[3];
        return true;
    }

    return false;
}

function getPersonFromScreen(person) {
    person.name = document.getElementById("fname").value;
    document.getElementById("fname").value = "";

    person.address = document.getElementById("faddress").value;
    document.getElementById("faddress").value = "";

    person.age = document.getElementById("fage").value;
    document.getElementById("fage").value = "";
}

function addPerson(person) {
    var table = document.getElementById("myTable");
    var row = table.insertRow(1);
    var idCell1 = row.insertCell(0);
    var nameCell2 = row.insertCell(1);
    var addressCell3 = row.insertCell(2)
    var ageCell3 = row.insertCell(3)

    idCell1.innerHTML = person.id;
    nameCell2.innerHTML = person.name;
    addressCell3.innerHTML = person.address;
    ageCell3.innerHTML = person.age;
}

function storePerson(person) {
    // Check browser support
    if (typeof (Storage) !== "undefined") {
        // Store
        localStorage.setItem("id", person.id);
        localStorage.setItem("name", person.name);
        localStorage.setItem("address", person.address);
        localStorage.setItem("age", person.age);
    }
}

function getPersonFromStorage(person) {
    // Check browser support
    if (typeof (Storage) !== "undefined") {
        // Retrieve
        person.id = localStorage.getItem("id");
        person.name = localStorage.getItem("name");
        person.address = localStorage.getItem("address");
        person.age = localStorage.getItem("age");
        id = person.id;
        return person;
    } else {
        alert("Browser does not support Web Storage.");
        return null;
    }
}

function getStorageData() {
    let person = new Person();
    getPersonFromStorage(person);
    setPersonFill(person);
}

function setPersonFill(person) {
    document.getElementById("fname").value = person.name;
    document.getElementById("faddress").value = person.address;
    document.getElementById("fage").value = person.age;
}

function getSelectedRowPerson() {

    if (row_selected === null) {
        alert("Please select a registry to edit");
        return;
    }

    let person = new Person();

    person.id = row_selected.cells[0].innerHTML;
    person.name = row_selected.cells[1].innerHTML;
    person.address = row_selected.cells[2].innerHTML;
    person.age = row_selected.cells[3].innerHTML;

    return person;
}

var row_selected;

function clearSelection() {
    var table = document.getElementById('myTable');
    var rows = table.querySelectorAll('tr');

    for (var i = 0; i < rows.length; i++) {
        rows[i].classList.remove('highlight');
    }
}

if(table != null)
{
    table.addEventListener('click', function (item) {

        // To get tr tag 
        // In the row where we click 	
        // item.defaultPrevented()			
        var row = item.path[1];
        row_selected = row;

        // Toggle the highlight 
        if (row.classList.contains('highlight')) {
            row.classList.remove('highlight');
            row_selected = null;
        }
        else {
            clearSelection();
            row.classList.add('highlight');
        }
    });
}


function handleGetData(dataReceived) {
    var objects = splitObject(dataReceived);
    for (let i = 0; i < objects.length; i++) {
        let person = new Person();
        if (createObject(objects[i], person) === true) {
            addPerson(person)
        }
    }
}

function returnHome(data) {
    data = data;
    window.location.replace('/');
}

function sendRequest(method, endpoint, data, callback){
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
            callback(xmlHttp.responseText);
        }
    }
    xmlHttp.open(method, endpoint, true); // true for asynchronous 
    xmlHttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    xmlHttp.send(data);
}

function sendSelectRequest(){
    sendRequest("GET", "/send", null, handleGetData);
}

function sendInsertRequest(){
    let person = new Person();
    getPersonFromScreen(person);
    let data = "fname=" + person.name.replace(/\s/g, '+') + "&faddress=" + person.address.replace(/\s/g, '+') + "&fage=" + person.age;
    sendRequest("POST", "/insert", data, returnHome);
}

function sendUpdateRequest(){
    let person = new Person();
    getPersonFromScreen(person);
    let data = "id=" + id + "&fname=" + person.name.replace(/\s/g, '+') + "&faddress=" + person.address.replace(/\s/g, '+') + "&fage=" + person.age;
    sendRequest("POST", "/update", data, returnHome);
}

function sendDeleteRequest(){
    let person = getSelectedRowPerson();
    if(person === null)
        return;
    let data = "id=" + person.id;
    sendRequest("POST", "/delete", data, returnHome);
}

function sendEdit() {
    let person = getSelectedRowPerson();
    if(person === null)
        return;
    storePerson(person)
    window.location.replace('/edit');
}

function sendNew() {
    window.location.replace('/new');
}
