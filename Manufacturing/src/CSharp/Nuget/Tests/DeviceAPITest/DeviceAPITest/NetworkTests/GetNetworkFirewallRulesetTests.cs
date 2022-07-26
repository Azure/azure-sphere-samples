/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Schema;

namespace TestDeviceRestAPI.NetworkTests
{
    /// <summary>
    /// A test class for the get network firewall ruleset endpoint.
    /// </summary>
    [TestClass]
    public class GetNetworkFirewallRulesetTests
    {
        /// <summary>Tests if getting the firewall ruleset returns rules in the correct format.</summary>
        [TestMethod]
        public void FirewallRuleset_Get_ReturnsList()
        {
            string expectedTopLevelSchema =
                @"{'type': 'object','properties': {'rulesets': {'type':'array', 'required':true}}}";
            string expectedRulesetSchema =
                @"{'type': 'object','properties': {'hook': {'type':'string'}, 'isValid':{'type': 'boolean'}, 'rules': {'type': 'array'}}}";
            string expectedRuleSchema =
                @"{'type': 'object','properties': {'sourceIP':{'type':'string'},'sourceMask':{'type':'string'},'destinationIP':{'type':'string'},'destinationMask':{'type':'string'},'uid':{'type':'integer'},'action':{'type':'string'},'interfaceInName':{'type':'string'},'interfaceOutName':{'type':'string'},'state':{'type':'string'},'tcpMask':{'type':'array'},'tcpCmp':{'type':'array'},'tcpInv':{'type':'boolean'},'protocol':{'type':'string'},'sourcePortRange':{'min':{'type':'integer'},'max':{'type':'integer'}},'destinationPortRange':{'min':{'type':'integer'},'max':{'type':'integer'}},'packets':{'type':'integer'},'bytes':{'type':'integer'}}}";
            string response = Network.GetNetworkFirewallRuleset();

            JObject rulesets = JObject.Parse(response);

            Assert.IsTrue(rulesets.IsValid(JSchema.Parse(expectedTopLevelSchema)));

            foreach (JObject ruleset in rulesets["rulesets"])
            {
                Assert.IsTrue(ruleset.IsValid(JSchema.Parse(expectedRulesetSchema)));
                JArray rules = (JArray)ruleset["rules"];
                foreach (JObject rule in rules)
                {
                    Assert.IsTrue(rule.IsValid(JSchema.Parse(expectedRuleSchema)));
                }
            }
        }
    }
}
