"use strict";

function listAccounts() {
    var statusDiv = document.getElementById("status");
    statusDiv.textContent = "loading accounts ...";
    var req = new XMLHttpRequest();
    req.open("GET", "http://localhost:8081/listAccounts", true);
    req.addEventListener("load", function() {
        statusDiv.textContent = req.status + ": " + req.statusText;
        if (req.status == 200) {
            var accountsDiv = document.getElementById("accounts");
            var new_table = document.createElement("tbody");
            var root = JSON.parse(req.responseText);
            root.accounts.map(function(account) {
                var row = new_table.insertRow(-1);
                row.insertCell(-1).innerHTML = account.id;
                row.insertCell(-1).innerHTML = account.description;
            });
            accountsDiv.replaceChild(new_table, document.querySelector("#accounts tbody"));
        }
    });
    req.addEventListener("error", function() {
        statusDiv.textContent = "Error in listAccounts (see javascript console)";
    });
    req.send();
}

function addAccount() {
    var accountName = document.getElementById("accountName");
    var statusDiv = document.getElementById("status");
    statusDiv.textContent = "Sending request ...";
    var req = new XMLHttpRequest();
    req.open("GET", "http://localhost:8081/addAccount?name=" + accountName.value, true);
    req.addEventListener("load", function() {
        statusDiv.textContent = req.status == 201 ? req.responseText : req.status + ": " + req.statusText;
        listAccounts();
    });
    req.addEventListener("error", function() {
        statusDiv.textContent = "Error in addAccount (see javascript console)";
        listAccounts();
    });
    req.send();
}
