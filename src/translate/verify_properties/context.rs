pub(crate) struct Definitions
{
	pub head_atom_parameters: std::rc::Rc<foliage::VariableDeclarations>,
	pub definitions: Vec<crate::ScopedFormula>,
}

type VariableDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::VariableDeclaration>, crate::Domain>;

type VariableDeclarationIDs
	= std::collections::BTreeMap::<std::rc::Rc<foliage::VariableDeclaration>, usize>;

pub(crate) struct Context
{
	pub definitions: std::cell::RefCell<std::collections::BTreeMap::<
		std::rc::Rc<foliage::PredicateDeclaration>, Definitions>>,
	pub integrity_constraints: std::cell::RefCell<foliage::Formulas>,

	pub function_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	pub variable_declaration_stack: std::cell::RefCell<foliage::VariableDeclarationStack>,
	pub variable_declaration_domains: std::cell::RefCell<VariableDeclarationDomains>,
	pub variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
}

impl Context
{
	pub(crate) fn new() -> Self
	{
		Self
		{
			definitions: std::cell::RefCell::new(std::collections::BTreeMap::<_, _>::new()),
			integrity_constraints: std::cell::RefCell::new(vec![]),

			function_declarations: std::cell::RefCell::new(foliage::FunctionDeclarations::new()),
			predicate_declarations: std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			variable_declaration_stack: std::cell::RefCell::new(foliage::VariableDeclarationStack::new()),
			variable_declaration_domains: std::cell::RefCell::new(VariableDeclarationDomains::new()),
			variable_declaration_ids: std::cell::RefCell::new(VariableDeclarationIDs::new()),
		}
	}
}

impl crate::translate::common::GetOrCreateFunctionDeclaration for Context
{
	fn get_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		let mut function_declarations = self.function_declarations.borrow_mut();

		match function_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::FunctionDeclaration::new(
					name.to_string(), arity));

				function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::translate::common::GetOrCreatePredicateDeclaration for Context
{
	fn get_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		let mut predicate_declarations = self.predicate_declarations.borrow_mut();

		match predicate_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::PredicateDeclaration::new(
					name.to_string(), arity));

				predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::translate::common::GetOrCreateVariableDeclaration for Context
{
	fn get_or_create_variable_declaration(&self, name: &str)
		-> std::rc::Rc<foliage::VariableDeclaration>
	{
		let mut variable_declaration_stack = self.variable_declaration_stack.borrow_mut();

		// TODO: check correctness
		if name == "_"
		{
			let variable_declaration = std::rc::Rc::new(foliage::VariableDeclaration::new(
				"_".to_owned()));

			variable_declaration_stack.free_variable_declarations.push(
				std::rc::Rc::clone(&variable_declaration));

			return variable_declaration;
		}

		variable_declaration_stack.find_or_create(name)
	}
}

impl crate::translate::common::AssignVariableDeclarationDomain for Context
{
	fn assign_variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>, domain: crate::Domain)
	{
		let mut variable_declaration_domains = self.variable_declaration_domains.borrow_mut();

		match variable_declaration_domains.get(variable_declaration)
		{
			Some(current_domain) => assert_eq!(*current_domain, domain,
				"inconsistent variable declaration domain"),
			None =>
			{
				variable_declaration_domains
					.insert(std::rc::Rc::clone(variable_declaration).into(), domain);
			},
		}
	}
}

impl crate::translate::common::VariableDeclarationDomain for Context
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> Option<crate::Domain>
	{
		let variable_declaration_domains = self.variable_declaration_domains.borrow();

		variable_declaration_domains.get(variable_declaration)
			.map(|x| *x)
	}
}

impl crate::translate::common::VariableDeclarationID for Context
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> usize
	{
		let mut variable_declaration_ids = self.variable_declaration_ids.borrow_mut();

		match variable_declaration_ids.get(variable_declaration)
		{
			Some(id) =>
			{
				*id
			}
			None =>
			{
				let id = variable_declaration_ids.len();
				variable_declaration_ids.insert(std::rc::Rc::clone(variable_declaration).into(), id);
				id
			},
		}
	}
}
